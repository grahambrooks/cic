use std::path::{Path, PathBuf};
use std::sync::Arc;

use anyhow::{anyhow, Context, Result};
use regex::Regex;
use reqwest::Client;
use serde::Deserialize;

use crate::providers::{github::GithubProvider, jenkins::JenkinsProvider, Provider};

#[derive(Debug, Deserialize)]
pub struct Config {
    #[serde(default = "default_refresh")]
    pub refresh_seconds: u64,
    #[serde(default = "default_notifications")]
    pub notifications: bool,
    #[serde(default)]
    pub providers: Vec<ProviderConfig>,
}

fn default_refresh() -> u64 {
    30
}

fn default_notifications() -> bool {
    true
}

#[derive(Debug, Deserialize)]
#[serde(tag = "type", rename_all = "lowercase")]
pub enum ProviderConfig {
    Jenkins {
        name: String,
        url: String,
        #[serde(default)]
        username: Option<String>,
        #[serde(default)]
        token: Option<String>,
        #[serde(default, rename = "match")]
        pattern: Option<String>,
        #[serde(default, rename = "match_project")]
        project_pattern: Option<String>,
    },
    Github {
        name: String,
        #[serde(default)]
        token: Option<String>,
        #[serde(default)]
        repos: Vec<String>,
        #[serde(default)]
        org: Option<String>,
        #[serde(default)]
        user: Option<String>,
        #[serde(default = "default_per_repo")]
        per_repo_limit: usize,
        #[serde(default, rename = "match")]
        pattern: Option<String>,
        #[serde(default, rename = "match_project")]
        project_pattern: Option<String>,
    },
}

fn default_per_repo() -> usize {
    10
}

impl Config {
    pub fn load(explicit: Option<&Path>) -> Result<Self> {
        let path = match explicit {
            Some(p) => p.to_path_buf(),
            None => discover_config_path().ok_or_else(|| {
                anyhow!(
                    "no config found; create ./cic.toml or ~/.config/cic/config.toml \
                     (or pass --config <path>)"
                )
            })?,
        };
        let text = std::fs::read_to_string(&path)
            .with_context(|| format!("reading {}", path.display()))?;
        let cfg: Config =
            toml::from_str(&text).with_context(|| format!("parsing {}", path.display()))?;
        Ok(cfg)
    }

    pub fn build_providers(&self, http: Client) -> Result<Vec<Arc<dyn Provider>>> {
        let mut out: Vec<Arc<dyn Provider>> = Vec::with_capacity(self.providers.len());
        for p in &self.providers {
            let provider: Arc<dyn Provider> = match p {
                ProviderConfig::Jenkins {
                    name,
                    url,
                    username,
                    token,
                    pattern,
                    project_pattern,
                } => {
                    let build_re = compile_pattern(name, "match", pattern.as_deref())?;
                    let project_re =
                        compile_pattern(name, "match_project", project_pattern.as_deref())?;
                    Arc::new(JenkinsProvider::new(
                        name.clone(),
                        url.clone(),
                        expand_env(username.as_deref()),
                        expand_env(token.as_deref()),
                        build_re,
                        project_re,
                        http.clone(),
                    ))
                }
                ProviderConfig::Github {
                    name,
                    token,
                    repos,
                    org,
                    user,
                    per_repo_limit,
                    pattern,
                    project_pattern,
                } => {
                    let build_re = compile_pattern(name, "match", pattern.as_deref())?;
                    let project_re =
                        compile_pattern(name, "match_project", project_pattern.as_deref())?;
                    Arc::new(GithubProvider::new(
                        name.clone(),
                        expand_env(token.as_deref()),
                        repos.clone(),
                        org.clone(),
                        user.clone(),
                        *per_repo_limit,
                        build_re,
                        project_re,
                        http.clone(),
                    ))
                }
            };
            out.push(provider);
        }
        Ok(out)
    }
}

fn compile_pattern(provider: &str, field: &str, pattern: Option<&str>) -> Result<Option<Regex>> {
    match pattern {
        Some(p) => Regex::new(p)
            .map(Some)
            .with_context(|| format!("invalid `{field}` regex for provider {provider}")),
        None => Ok(None),
    }
}

fn discover_config_path() -> Option<PathBuf> {
    let local = PathBuf::from("cic.toml");
    if local.is_file() {
        return Some(local);
    }
    if let Some(home) = dirs::config_dir() {
        let p = home.join("cic").join("config.toml");
        if p.is_file() {
            return Some(p);
        }
    }
    None
}

/// If the value is `${ENV_VAR}` (a single env reference), substitute it. Plain values pass through.
fn expand_env(value: Option<&str>) -> Option<String> {
    let v = value?.trim();
    if let Some(name) = v.strip_prefix("${").and_then(|s| s.strip_suffix('}')) {
        std::env::var(name).ok()
    } else {
        Some(v.to_string())
    }
}
