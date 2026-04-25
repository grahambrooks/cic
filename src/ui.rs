use chrono::Utc;
use ratatui::layout::{Constraint, Direction, Layout, Rect};
use ratatui::style::{Color, Modifier, Style};
use ratatui::text::{Line, Span};
use ratatui::widgets::{Block, BorderType, Borders, Paragraph};
use ratatui::Frame;

use crate::app::App;
use crate::providers::{Build, BuildStatus};

const CARD_MIN_WIDTH: u16 = 32;
const CARD_HEIGHT: u16 = 5;

/// Render the whole UI; returns the column count used for the grid so the
/// event loop can translate up/down keys into the right index delta.
pub fn draw(f: &mut Frame, app: &App) -> usize {
    let chunks = Layout::default()
        .direction(Direction::Vertical)
        .constraints([
            Constraint::Length(1),
            Constraint::Min(CARD_HEIGHT),
            Constraint::Length(if app.errors.is_empty() { 1 } else { 4 }),
        ])
        .split(f.area());

    draw_header(f, app, chunks[0]);
    let columns = draw_grid(f, app, chunks[1]);
    draw_footer(f, app, chunks[2]);
    columns.max(1)
}

fn draw_header(f: &mut Frame, app: &App, area: Rect) {
    let last = app
        .last_refresh
        .map(|t| {
            let ago = Utc::now().signed_duration_since(t).num_seconds().max(0);
            format!("updated {ago}s ago")
        })
        .unwrap_or_else(|| "no data yet".to_string());

    let refreshing = if app.refreshing { "  ·  refreshing" } else { "" };

    let header = Line::from(vec![
        Span::styled(
            "cic",
            Style::default().fg(Color::Cyan).add_modifier(Modifier::BOLD),
        ),
        Span::raw("  ·  "),
        Span::raw(last),
        Span::styled(refreshing, Style::default().fg(Color::Yellow)),
    ]);
    f.render_widget(Paragraph::new(header), area);
}

fn draw_grid(f: &mut Frame, app: &App, area: Rect) -> usize {
    if app.projects.is_empty() {
        let msg = if app.refreshing {
            "loading…"
        } else {
            "no projects to show — check provider config / regex"
        };
        f.render_widget(
            Paragraph::new(Span::styled(msg, Style::default().fg(Color::DarkGray)))
                .block(Block::default().borders(Borders::ALL).title(" projects ")),
            area,
        );
        return 1;
    }

    let columns = (area.width / CARD_MIN_WIDTH).max(1) as usize;
    let total_rows = app.projects.len().div_ceil(columns);

    let row_constraints: Vec<Constraint> =
        (0..total_rows).map(|_| Constraint::Length(CARD_HEIGHT)).collect();
    let row_areas = Layout::default()
        .direction(Direction::Vertical)
        .constraints(row_constraints)
        .split(area);

    let col_constraints: Vec<Constraint> =
        (0..columns).map(|_| Constraint::Ratio(1, columns as u32)).collect();

    for (row_idx, row_area) in row_areas.iter().enumerate() {
        let cell_areas = Layout::default()
            .direction(Direction::Horizontal)
            .constraints(col_constraints.clone())
            .split(*row_area);

        for (col_idx, cell_area) in cell_areas.iter().enumerate() {
            let project_idx = row_idx * columns + col_idx;
            let Some(build) = app.projects.get(project_idx) else { break };
            draw_card(f, *cell_area, build, project_idx == app.selected);
        }
    }
    columns
}

fn draw_card(f: &mut Frame, area: Rect, build: &Build, selected: bool) {
    let color = status_color(build.status);
    let (border_color, border_type) = if selected {
        (Color::White, BorderType::Double)
    } else {
        (color, BorderType::Plain)
    };

    let title_style = if selected {
        Style::default()
            .fg(Color::Black)
            .bg(Color::White)
            .add_modifier(Modifier::BOLD)
    } else {
        Style::default().fg(color).add_modifier(Modifier::BOLD)
    };

    let title = format!(" {} ", build.project);
    let block = Block::default()
        .borders(Borders::ALL)
        .border_type(border_type)
        .border_style(Style::default().fg(border_color))
        .title(Span::styled(title, title_style));

    let mut lines = vec![
        Line::from(vec![
            Span::styled(
                build.status.label().to_uppercase(),
                Style::default().fg(color).add_modifier(Modifier::BOLD),
            ),
            Span::raw("   "),
            Span::styled(build.provider.clone(), Style::default().fg(Color::DarkGray)),
        ]),
        Line::from(Span::styled(
            build.name.clone(),
            Style::default().fg(Color::White),
        )),
    ];
    if let Some(branch) = &build.branch {
        lines.push(Line::from(Span::styled(
            format!("⤷ {branch}"),
            Style::default().fg(Color::DarkGray),
        )));
    }

    f.render_widget(Paragraph::new(lines).block(block), area);
}

fn draw_footer(f: &mut Frame, app: &App, area: Rect) {
    let chunks = Layout::default()
        .direction(Direction::Vertical)
        .constraints(if app.errors.is_empty() {
            vec![Constraint::Length(1)]
        } else {
            vec![Constraint::Min(1), Constraint::Length(1)]
        })
        .split(area);

    if !app.errors.is_empty() {
        let lines: Vec<Line> = app
            .errors
            .iter()
            .map(|e| Line::from(Span::styled(e.clone(), Style::default().fg(Color::Red))))
            .collect();
        f.render_widget(
            Paragraph::new(lines)
                .block(Block::default().borders(Borders::ALL).title(" errors ")),
            chunks[0],
        );
    }

    let last_idx = chunks.len() - 1;
    let help = Line::from(vec![
        Span::styled(app.status_line.clone(), Style::default().fg(Color::Gray)),
        Span::raw("   "),
        Span::styled("[q]", Style::default().fg(Color::Cyan)),
        Span::raw(" quit  "),
        Span::styled("[r]", Style::default().fg(Color::Cyan)),
        Span::raw(" refresh  "),
        Span::styled("[hjkl/arrows]", Style::default().fg(Color::Cyan)),
        Span::raw(" select  "),
        Span::styled("[o]", Style::default().fg(Color::Cyan)),
        Span::raw(" open"),
    ]);
    f.render_widget(Paragraph::new(help), chunks[last_idx]);
}

fn status_color(s: BuildStatus) -> Color {
    match s {
        BuildStatus::Success => Color::Green,
        BuildStatus::Failure => Color::Red,
        BuildStatus::Unstable => Color::Yellow,
        BuildStatus::Running => Color::Cyan,
        BuildStatus::Queued => Color::Magenta,
        BuildStatus::Cancelled => Color::DarkGray,
        BuildStatus::NotBuilt => Color::DarkGray,
        BuildStatus::Unknown => Color::Gray,
    }
}
