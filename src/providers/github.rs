use anyhow::{Context, Result};
use async_trait::async_trait;
use futures::future::try_join_all;
use regex::Regex;
use reqwest::Client;
use serde::Deserialize;

use super::{Build, BuildStatus, Provider};

const REPO_PAGE_SIZE: usize = 100;
const REPO_PAGE_LIMIT: usize = 5;

pub struct GithubProvider {
    name: String,
    token: Option<String>,
    repos: Vec<String>,
    org: Option<String>,
    user: Option<String>,
    per_repo_limit: usize,
    pattern: Option<Regex>,
    project_pattern: Option<Regex>,
    http: Client,
}

impl GithubProvider {
    #[allow(clippy::too_many_arguments)]
    pub fn new(
        name: String,
        token: Option<String>,
        repos: Vec<String>,
        org: Option<String>,
        user: Option<String>,
        per_repo_limit: usize,
        pattern: Option<Regex>,
        project_pattern: Option<Regex>,
        http: Client,
    ) -> Self {
        Self {
            name,
            token,
            repos,
            org,
            user,
            per_repo_limit,
            pattern,
            project_pattern,
            http,
        }
    }
}

#[derive(Debug, Deserialize)]
struct RunsResponse {
    workflow_runs: Vec<Run>,
}

#[derive(Debug, Deserialize)]
struct Run {
    name: Option<String>,
    head_branch: Option<String>,
    status: Option<String>,
    conclusion: Option<String>,
    html_url: String,
}

#[derive(Debug, Deserialize)]
struct RepoSummary {
    full_name: String,
}

#[async_trait]
impl Provider for GithubProvider {
    fn name(&self) -> &str {
        &self.name
    }

    async fn fetch_builds(&self) -> Result<Vec<Build>> {
        let repos = self.resolve_repos().await?;
        if repos.is_empty() {
            return Ok(Vec::new());
        }

        let limit = self.per_repo_limit.max(1);
        let fetches = repos.iter().map(|repo| self.fetch_repo(repo, limit));
        let nested = try_join_all(fetches).await?;
        Ok(nested.into_iter().flatten().collect())
    }
}

impl GithubProvider {
    /// Combine explicit repos with any discovered from `org`/`user`, dedupe, and
    /// apply the project regex.
    async fn resolve_repos(&self) -> Result<Vec<String>> {
        let mut repos = self.repos.clone();
        if let Some(org) = &self.org {
            repos.extend(self.list_repos(&format!("orgs/{org}/repos")).await?);
        }
        if let Some(user) = &self.user {
            repos.extend(self.list_repos(&format!("users/{user}/repos")).await?);
        }
        repos.sort();
        repos.dedup();

        if let Some(re) = &self.project_pattern {
            repos.retain(|r| re.is_match(r));
        }
        Ok(repos)
    }

    async fn list_repos(&self, path_prefix: &str) -> Result<Vec<String>> {
        let mut out = Vec::new();
        for page in 1..=REPO_PAGE_LIMIT {
            let url = format!(
                "https://api.github.com/{path_prefix}\
                 ?per_page={REPO_PAGE_SIZE}&page={page}&type=all"
            );
            let body: Vec<RepoSummary> = self
                .request(&url)
                .send()
                .await
                .with_context(|| format!("GET {url}"))?
                .error_for_status()
                .with_context(|| format!("listing repos at {path_prefix}"))?
                .json()
                .await
                .with_context(|| format!("decode repo list from {path_prefix}"))?;
            let len = body.len();
            out.extend(body.into_iter().map(|r| r.full_name));
            if len < REPO_PAGE_SIZE {
                break;
            }
        }
        Ok(out)
    }

    async fn fetch_repo(&self, repo: &str, limit: usize) -> Result<Vec<Build>> {
        let url = format!(
            "https://api.github.com/repos/{repo}/actions/runs?per_page={limit}"
        );
        let body: RunsResponse = self
            .request(&url)
            .send()
            .await
            .with_context(|| format!("GET {url}"))?
            .error_for_status()
            .with_context(|| format!("GitHub repo {repo} returned an error"))?
            .json()
            .await
            .context("decode GitHub response")?;

        let builds = body
            .workflow_runs
            .into_iter()
            .map(|r| Build {
                provider: self.name.clone(),
                project: repo.to_string(),
                name: r.name.unwrap_or_else(|| "workflow".to_string()),
                branch: r.head_branch,
                status: run_to_status(r.status.as_deref(), r.conclusion.as_deref()),
                url: r.html_url,
            })
            .filter(|b| match &self.pattern {
                Some(re) => re.is_match(&b.name),
                None => true,
            })
            .collect();
        Ok(builds)
    }

    fn request(&self, url: &str) -> reqwest::RequestBuilder {
        let mut req = self
            .http
            .get(url)
            .header("Accept", "application/vnd.github+json")
            .header("X-GitHub-Api-Version", "2022-11-28");
        if let Some(t) = &self.token {
            req = req.bearer_auth(t);
        }
        req
    }
}

fn run_to_status(status: Option<&str>, conclusion: Option<&str>) -> BuildStatus {
    match status {
        Some("queued") | Some("waiting") | Some("requested") | Some("pending") => {
            BuildStatus::Queued
        }
        Some("in_progress") => BuildStatus::Running,
        Some("completed") => match conclusion {
            Some("success") => BuildStatus::Success,
            Some("failure") | Some("timed_out") | Some("startup_failure") => BuildStatus::Failure,
            Some("cancelled") | Some("skipped") => BuildStatus::Cancelled,
            Some("neutral") | Some("action_required") => BuildStatus::Unstable,
            _ => BuildStatus::Unknown,
        },
        _ => BuildStatus::Unknown,
    }
}
