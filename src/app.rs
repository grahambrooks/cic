use std::collections::{HashMap, HashSet};
use std::sync::Arc;
use std::time::{Duration, Instant};

use chrono::{DateTime, Utc};
use futures::future::join_all;
use tokio::sync::mpsc;

use crate::providers::{Build, BuildStatus, Provider};

type ProjectKey = (String, String);

pub struct App {
    pub providers: Vec<Arc<dyn Provider>>,
    pub projects: Vec<Build>,
    pub errors: Vec<String>,
    pub last_refresh: Option<DateTime<Utc>>,
    pub refresh_every: Duration,
    pub selected: usize,
    pub status_line: String,
    pub refreshing: bool,
    pub should_quit: bool,
    pub notifications_enabled: bool,
    /// Status from the last refresh, keyed by (provider, project). Used to detect
    /// red transitions for notifications. Empty until the first refresh applies,
    /// so the initial fetch never fires notifications.
    previous_status: HashMap<ProjectKey, BuildStatus>,
    has_completed_first_refresh: bool,
}

pub enum FetchOutcome {
    Ok { builds: Vec<Build> },
    Err { provider: String, error: String },
}

#[derive(Debug, Clone, Copy)]
pub enum Move {
    Left,
    Right,
    Up,
    Down,
}

impl App {
    pub fn new(
        providers: Vec<Arc<dyn Provider>>,
        refresh_every: Duration,
        notifications_enabled: bool,
    ) -> Self {
        Self {
            providers,
            projects: Vec::new(),
            errors: Vec::new(),
            last_refresh: None,
            refresh_every,
            selected: 0,
            status_line: "starting…".into(),
            refreshing: false,
            should_quit: false,
            notifications_enabled,
            previous_status: HashMap::new(),
            has_completed_first_refresh: false,
        }
    }

    pub fn move_selection(&mut self, m: Move, columns: usize) {
        if self.projects.is_empty() {
            return;
        }
        let len = self.projects.len();
        let cols = columns.max(1);
        let cur = self.selected;
        self.selected = match m {
            Move::Left => {
                if cur == 0 {
                    len - 1
                } else {
                    cur - 1
                }
            }
            Move::Right => (cur + 1) % len,
            Move::Up => cur.saturating_sub(cols),
            Move::Down => {
                let next = cur + cols;
                if next < len {
                    next
                } else {
                    cur
                }
            }
        };
    }

    pub fn begin_refresh(&mut self, tx: mpsc::UnboundedSender<FetchOutcome>) {
        if self.refreshing {
            return;
        }
        self.refreshing = true;
        self.status_line = "refreshing…".into();
        let providers = self.providers.clone();
        tokio::spawn(async move {
            let futs = providers.into_iter().map(|p| {
                let tx = tx.clone();
                async move {
                    let name = p.name().to_string();
                    let outcome = match p.fetch_builds().await {
                        Ok(builds) => FetchOutcome::Ok { builds },
                        Err(e) => FetchOutcome::Err { provider: name, error: format!("{e:#}") },
                    };
                    let _ = tx.send(outcome);
                }
            });
            join_all(futs).await;
        });
    }

    pub fn apply_outcomes(&mut self, outcomes: Vec<FetchOutcome>) {
        let mut builds: Vec<Build> = Vec::new();
        let mut errors: Vec<String> = Vec::new();
        for o in outcomes {
            match o {
                FetchOutcome::Ok { builds: mut b } => builds.append(&mut b),
                FetchOutcome::Err { provider, error } => {
                    errors.push(format!("{provider}: {error}"));
                }
            }
        }

        let mut projects = latest_per_project(&builds);
        projects.sort_by(|a, b| {
            a.status
                .sort_priority()
                .cmp(&b.status.sort_priority())
                .then_with(|| a.provider.cmp(&b.provider))
                .then_with(|| a.project.cmp(&b.project))
        });

        // Remember which project the user was on so we can pin the cursor across
        // sort changes.
        let prior_selection: Option<ProjectKey> = self
            .projects
            .get(self.selected)
            .map(|b| (b.provider.clone(), b.project.clone()));

        // Detect red transitions before we overwrite previous_status.
        if self.notifications_enabled && self.has_completed_first_refresh {
            for project in &projects {
                let key = (project.provider.clone(), project.project.clone());
                let prev = self.previous_status.get(&key).copied();
                if matches!(project.status, BuildStatus::Failure)
                    && !matches!(prev, Some(BuildStatus::Failure))
                {
                    notify_red(project);
                }
            }
        }

        // Refresh previous_status to current.
        self.previous_status = projects
            .iter()
            .map(|b| ((b.provider.clone(), b.project.clone()), b.status))
            .collect();

        self.projects = projects;
        self.errors = errors;
        self.last_refresh = Some(Utc::now());
        self.refreshing = false;
        self.has_completed_first_refresh = true;

        self.selected = match prior_selection {
            Some(key) => self
                .projects
                .iter()
                .position(|b| b.provider == key.0 && b.project == key.1)
                .unwrap_or_else(|| self.selected.min(self.projects.len().saturating_sub(1))),
            None => 0,
        };

        let red = self
            .projects
            .iter()
            .filter(|p| matches!(p.status, BuildStatus::Failure))
            .count();
        self.status_line = format!(
            "{} project(s)  ·  {} red  ·  {} error(s)",
            self.projects.len(),
            red,
            self.errors.len()
        );
    }

    pub fn due_for_refresh(&self, last_tick: Instant) -> bool {
        !self.refreshing && last_tick.elapsed() >= self.refresh_every
    }

    pub fn selected_url(&self) -> Option<&str> {
        self.projects.get(self.selected).map(|b| b.url.as_str())
    }
}

/// Keep the first build encountered per (provider, project). Providers must yield
/// builds in newest-first order for "first" to mean "latest".
fn latest_per_project(builds: &[Build]) -> Vec<Build> {
    let mut seen: HashSet<ProjectKey> = HashSet::new();
    let mut out = Vec::new();
    for b in builds {
        let key = (b.provider.clone(), b.project.clone());
        if seen.insert(key) {
            out.push(b.clone());
        }
    }
    out
}

fn notify_red(build: &Build) {
    let summary = format!("{} failed", build.project);
    let body = format!("{} · {}", build.provider, build.name);
    // notify-rust returns Err on platforms without a notification daemon; we
    // ignore those — the failure indicator is already on screen.
    let _ = notify_rust::Notification::new()
        .summary(&summary)
        .body(&body)
        .appname("cic")
        .show();
}

pub fn drain_outcomes(rx: &mut mpsc::UnboundedReceiver<FetchOutcome>) -> Vec<FetchOutcome> {
    let mut out = Vec::new();
    while let Ok(o) = rx.try_recv() {
        out.push(o);
    }
    out
}
