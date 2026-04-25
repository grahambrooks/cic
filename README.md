# cic — CI console

A terminal UI for monitoring builds across **GitHub Actions** and **Jenkins**
in one place. The screen is a grid of cards: one card per project, showing the
latest build's status. Border colour reflects status at a glance. Failures
sort to the top, and a desktop notification fires when any project transitions
to red.

```
┌ owner/repo-one ────────────────┐ ┌ owner/repo-two ────────────────┐
│ SUCCESS    my-repos            │ │ RUNNING    my-repos            │
│ CI                             │ │ Release                        │
│ ⤷ main                         │ │ ⤷ main                         │
└────────────────────────────────┘ └────────────────────────────────┘
┌ team-alpha-api ────────────────┐ ┌ team-alpha-web ────────────────┐
│ FAILED     company-jenkins     │ │ SUCCESS    company-jenkins     │
│ team-alpha-api                 │ │ team-alpha-web                 │
└────────────────────────────────┘ └────────────────────────────────┘
```

## Install

### Homebrew (macOS / Linux)

```sh
brew tap grahambrooks/cic https://github.com/grahambrooks/cic.git
brew install cic
```

The tap pulls the formula from `Formula/cic.rb` in this repository, which is
auto-regenerated on every release.

### Pre-built binaries

Each release attaches archives for these targets to the GitHub release page:

- `aarch64-apple-darwin` (macOS Apple Silicon, `.tar.gz`)
- `x86_64-apple-darwin`  (macOS Intel, `.tar.gz`)
- `x86_64-unknown-linux-gnu` (Linux x86_64, `.tar.gz`)
- `x86_64-pc-windows-msvc`   (Windows x86_64, `.zip`)

### From source

```sh
cargo build --release
./target/release/cic --help
```

## Configure

`cic` looks for, in order:

1. `--config <path>`
2. `./cic.toml`
3. `~/.config/cic/config.toml`

Copy `cic.example.toml` and edit it. Tokens may be inlined or referenced as
`${ENV_VAR}`.

Each provider takes two optional regexes:

| Field            | Filters                                      |
|------------------|----------------------------------------------|
| `match_project`  | Project name (Jenkins job, GitHub `owner/name`) |
| `match`          | Build name (Jenkins job, GitHub workflow run name) |

Both default to "match everything" when omitted. They AND together.

GitHub builds the project list by combining explicit `repos` with anything
discovered via the optional `org` / `user` sources, then applies
`match_project`. Up to 500 repos per source are paged in.

Top-level options:

| Field             | Default | Notes                                              |
|-------------------|---------|----------------------------------------------------|
| `refresh_seconds` | `30`    | Polling interval (overridable with `--refresh`).   |
| `notifications`   | `true`  | Desktop notification on transitions to red. Set to `false` to silence. |

```toml
refresh_seconds = 30
notifications   = true

[[providers]]
type = "github"
name = "my-repos"
token = "${GITHUB_TOKEN}"
repos = ["owner/repo-extra"]              # explicit, optional
org   = "my-company"                      # discover from an org
# user = "grahambrooks"                   # …or a user
match_project = "^my-company/(api|web)-"  # filter the merged list
match         = "^(CI|Release)$"          # filter workflow runs

[[providers]]
type = "jenkins"
name = "company-jenkins"
url = "https://jenkins.example.com"
username = "${JENKINS_USER}"
token = "${JENKINS_TOKEN}"
match_project = "^team-alpha-"            # job name == project name on Jenkins
```

GitHub tokens need `repo` (or `actions:read` and `metadata:read` for
fine-grained tokens). Jenkins auth uses an API token over HTTP Basic.

## Run

```sh
cic                       # use discovered config
cic --config ./other.toml
cic --refresh 10          # override refresh interval (seconds)
```

### Keys

| Key                | Action                                     |
|--------------------|--------------------------------------------|
| `q` / Esc          | Quit                                       |
| `r`                | Refresh now                                |
| `h` / `←`          | Select cell to the left                    |
| `l` / `→`          | Select cell to the right                   |
| `k` / `↑`          | Select cell above                          |
| `j` / `↓`          | Select cell below                          |
| `o`                | Open selected build in your browser        |

The grid auto-fits the terminal width — resize and the columns reflow.

### Logging

Logs are silenced by default. Set `CIC_LOG=info` (or any
`tracing-subscriber` filter) to send logs to stderr.

## Releasing

Releases follow a CalVer scheme: `YYYY.M.D` (e.g. `2026.4.25`, no
zero-padding). Pushing a matching tag triggers
`.github/workflows/release.yml`, which:

1. Builds binaries for the four supported targets.
2. Packages each as a `.tar.gz` (or `.zip` on Windows) and attaches them to a
   GitHub release.
3. Regenerates `Formula/cic.rb` with the new version + sha256 values and
   commits the change back to `master`.

```sh
ver=$(date +%Y.%-m.%-d)
git tag "$ver"
git push origin "$ver"
```

## Status

Things deliberately still out of scope:

- per-build details / log viewer (drill-down on Enter)
- in-TUI search / filter (`/`)
- build duration on running cards
- additional providers (CircleCI, GitLab, Buildkite)
