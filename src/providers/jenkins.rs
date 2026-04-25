use anyhow::{Context, Result};
use async_trait::async_trait;
use regex::Regex;
use reqwest::Client;
use serde::Deserialize;

use super::{Build, BuildStatus, Provider};

pub struct JenkinsProvider {
    name: String,
    base_url: String,
    username: Option<String>,
    token: Option<String>,
    pattern: Option<Regex>,
    project_pattern: Option<Regex>,
    http: Client,
}

impl JenkinsProvider {
    #[allow(clippy::too_many_arguments)]
    pub fn new(
        name: String,
        base_url: String,
        username: Option<String>,
        token: Option<String>,
        pattern: Option<Regex>,
        project_pattern: Option<Regex>,
        http: Client,
    ) -> Self {
        let base_url = base_url.trim_end_matches('/').to_string();
        Self {
            name,
            base_url,
            username,
            token,
            pattern,
            project_pattern,
            http,
        }
    }
}

#[derive(Debug, Deserialize)]
struct JobsResponse {
    jobs: Vec<Job>,
}

#[derive(Debug, Deserialize)]
struct Job {
    name: String,
    #[serde(default)]
    color: Option<String>,
    #[serde(default)]
    url: Option<String>,
}

#[async_trait]
impl Provider for JenkinsProvider {
    fn name(&self) -> &str {
        &self.name
    }

    async fn fetch_builds(&self) -> Result<Vec<Build>> {
        let url = format!("{}/api/json?tree=jobs[name,color,url]", self.base_url);
        let mut req = self.http.get(&url);
        if let (Some(u), Some(t)) = (&self.username, &self.token) {
            req = req.basic_auth(u, Some(t));
        }
        let resp = req
            .send()
            .await
            .with_context(|| format!("GET {url}"))?
            .error_for_status()
            .with_context(|| format!("Jenkins {} returned an error", self.name))?;

        let body: JobsResponse = resp.json().await.context("decode Jenkins response")?;

        let builds = body
            .jobs
            .into_iter()
            .filter(|j| matches_opt(self.project_pattern.as_ref(), &j.name))
            .filter(|j| matches_opt(self.pattern.as_ref(), &j.name))
            .map(|j| Build {
                provider: self.name.clone(),
                project: j.name.clone(),
                name: j.name.clone(),
                branch: None,
                status: color_to_status(j.color.as_deref()),
                url: j
                    .url
                    .unwrap_or_else(|| format!("{}/job/{}/", self.base_url, j.name)),
            })
            .collect();

        Ok(builds)
    }
}

fn matches_opt(re: Option<&Regex>, value: &str) -> bool {
    re.is_none_or(|r| r.is_match(value))
}

fn color_to_status(color: Option<&str>) -> BuildStatus {
    let Some(c) = color else {
        return BuildStatus::Unknown;
    };
    let building = c.contains("anime");
    let base = c.split('_').next().unwrap_or(c);
    match (base, building) {
        (_, true) => BuildStatus::Running,
        ("blue", _) => BuildStatus::Success,
        ("red", _) => BuildStatus::Failure,
        ("yellow", _) => BuildStatus::Unstable,
        ("aborted", _) => BuildStatus::Cancelled,
        ("notbuilt", _) | ("disabled", _) => BuildStatus::NotBuilt,
        _ => BuildStatus::Unknown,
    }
}
