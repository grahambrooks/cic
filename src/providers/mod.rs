use anyhow::Result;
use async_trait::async_trait;
use serde::{Deserialize, Serialize};

pub mod github;
pub mod jenkins;

#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
pub enum BuildStatus {
    Success,
    Failure,
    Unstable,
    Running,
    Queued,
    Cancelled,
    NotBuilt,
    Unknown,
}

impl BuildStatus {
    pub fn label(self) -> &'static str {
        match self {
            BuildStatus::Success => "success",
            BuildStatus::Failure => "failed",
            BuildStatus::Unstable => "unstable",
            BuildStatus::Running => "running",
            BuildStatus::Queued => "queued",
            BuildStatus::Cancelled => "cancelled",
            BuildStatus::NotBuilt => "not built",
            BuildStatus::Unknown => "unknown",
        }
    }

    /// Lower = more urgent. Used to sort the grid so red appears first.
    pub fn sort_priority(self) -> u8 {
        match self {
            BuildStatus::Failure => 0,
            BuildStatus::Unstable => 1,
            BuildStatus::Running => 2,
            BuildStatus::Queued => 3,
            BuildStatus::Unknown => 4,
            BuildStatus::Success => 5,
            BuildStatus::Cancelled => 6,
            BuildStatus::NotBuilt => 7,
        }
    }
}

#[derive(Debug, Clone)]
pub struct Build {
    pub provider: String,
    pub project: String,
    pub name: String,
    pub branch: Option<String>,
    pub status: BuildStatus,
    pub url: String,
}

#[async_trait]
pub trait Provider: Send + Sync {
    fn name(&self) -> &str;
    async fn fetch_builds(&self) -> Result<Vec<Build>>;
}
