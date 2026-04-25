mod app;
mod config;
mod providers;
mod ui;

use std::io::{stdout, Stdout};
use std::path::PathBuf;
use std::time::{Duration, Instant};

use anyhow::{Context, Result};
use clap::Parser;
use crossterm::event::{Event, EventStream, KeyCode, KeyEventKind, KeyModifiers};
use crossterm::execute;
use crossterm::terminal::{
    disable_raw_mode, enable_raw_mode, EnterAlternateScreen, LeaveAlternateScreen,
};
use futures::StreamExt;
use ratatui::backend::CrosstermBackend;
use ratatui::Terminal;
use tokio::sync::mpsc;
use tokio::time::sleep;

use crate::app::{drain_outcomes, App, FetchOutcome, Move};
use crate::config::Config;

#[derive(Parser, Debug)]
#[command(name = "cic", about = "Terminal UI for monitoring CI builds")]
struct Cli {
    /// Path to config file (default: ./cic.toml or ~/.config/cic/config.toml)
    #[arg(short, long)]
    config: Option<PathBuf>,

    /// Override refresh interval in seconds
    #[arg(short, long)]
    refresh: Option<u64>,
}

#[tokio::main]
async fn main() -> Result<()> {
    init_tracing();

    let cli = Cli::parse();
    let cfg = Config::load(cli.config.as_deref())?;

    let http = reqwest::Client::builder()
        .user_agent(concat!("cic/", env!("CARGO_PKG_VERSION")))
        .timeout(Duration::from_secs(15))
        .build()
        .context("build HTTP client")?;

    let providers = cfg.build_providers(http)?;
    if providers.is_empty() {
        anyhow::bail!("config has no [[providers]] entries");
    }

    let refresh = Duration::from_secs(cli.refresh.unwrap_or(cfg.refresh_seconds).max(1));
    let app = App::new(providers, refresh, cfg.notifications);

    let mut terminal = setup_terminal()?;
    let result = run(&mut terminal, app).await;
    restore_terminal(&mut terminal)?;
    result
}

fn init_tracing() {
    // Logs go to stderr only when CIC_LOG is set, so they don't trash the TUI by default.
    if std::env::var_os("CIC_LOG").is_some() {
        let _ = tracing_subscriber::fmt()
            .with_env_filter(
                tracing_subscriber::EnvFilter::try_from_env("CIC_LOG")
                    .unwrap_or_else(|_| tracing_subscriber::EnvFilter::new("info")),
            )
            .with_writer(std::io::stderr)
            .try_init();
    }
}

type Term = Terminal<CrosstermBackend<Stdout>>;

fn setup_terminal() -> Result<Term> {
    enable_raw_mode().context("enable raw mode")?;
    let mut out = stdout();
    execute!(out, EnterAlternateScreen).context("enter alternate screen")?;
    let backend = CrosstermBackend::new(out);
    Terminal::new(backend).context("init terminal")
}

fn restore_terminal(terminal: &mut Term) -> Result<()> {
    disable_raw_mode().ok();
    execute!(terminal.backend_mut(), LeaveAlternateScreen).ok();
    terminal.show_cursor().ok();
    Ok(())
}

async fn run(terminal: &mut Term, mut app: App) -> Result<()> {
    let (tx, mut rx) = mpsc::unbounded_channel::<FetchOutcome>();
    app.begin_refresh(tx.clone());
    let mut last_refresh_kicked = Instant::now();
    let mut columns: usize = 1;

    let mut events = EventStream::new();
    let tick_rate = Duration::from_millis(200);

    loop {
        terminal.draw(|f| {
            columns = ui::draw(f, &app);
        })?;

        let outcomes = drain_outcomes(&mut rx);
        if !outcomes.is_empty() {
            app.apply_outcomes(outcomes);
        }

        if app.due_for_refresh(last_refresh_kicked) {
            app.begin_refresh(tx.clone());
            last_refresh_kicked = Instant::now();
        }

        let tick = sleep(tick_rate);
        tokio::pin!(tick);

        tokio::select! {
            maybe_event = events.next() => {
                match maybe_event {
                    Some(Ok(Event::Key(key))) if key.kind == KeyEventKind::Press => {
                        match (key.code, key.modifiers) {
                            (KeyCode::Char('q'), _) | (KeyCode::Esc, _) => app.should_quit = true,
                            (KeyCode::Char('c'), KeyModifiers::CONTROL) => app.should_quit = true,
                            (KeyCode::Char('r'), _) => {
                                app.begin_refresh(tx.clone());
                                last_refresh_kicked = Instant::now();
                            }
                            (KeyCode::Right, _) | (KeyCode::Char('l'), _) => {
                                app.move_selection(Move::Right, columns);
                            }
                            (KeyCode::Left, _) | (KeyCode::Char('h'), _) => {
                                app.move_selection(Move::Left, columns);
                            }
                            (KeyCode::Down, _) | (KeyCode::Char('j'), _) => {
                                app.move_selection(Move::Down, columns);
                            }
                            (KeyCode::Up, _) | (KeyCode::Char('k'), _) => {
                                app.move_selection(Move::Up, columns);
                            }
                            (KeyCode::Char('o'), _) => open_selected(&app),
                            _ => {}
                        }
                    }
                    Some(Err(e)) => {
                        app.errors.push(format!("event error: {e}"));
                    }
                    None => app.should_quit = true,
                    _ => {}
                }
            }
            _ = &mut tick => {}
        }

        if app.should_quit {
            return Ok(());
        }
    }
}

fn open_selected(app: &App) {
    let Some(url) = app.selected_url().map(str::to_string) else { return };
    tokio::spawn(async move {
        #[cfg(target_os = "macos")]
        let _ = std::process::Command::new("open").arg(&url).status();
        #[cfg(target_os = "linux")]
        let _ = std::process::Command::new("xdg-open").arg(&url).status();
        #[cfg(target_os = "windows")]
        let _ = std::process::Command::new("cmd").args(["/C", "start", &url]).status();
    });
}
