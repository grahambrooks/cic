# Standard targets. `make check` is the pre-push gate and runs exactly what the
# CI check job runs (.github/workflows/ci.yml); `make hooks` installs the git hooks.
.DEFAULT_GOAL := help

CARGO ?= cargo

.PHONY: help check fmt fmt-check lint test hooks

help: ## List targets
	@awk 'BEGIN {FS = ":.*?## "} /^[a-zA-Z_-]+:.*?## / {printf "  \033[36m%-16s\033[0m %s\n", $$1, $$2}' $(MAKEFILE_LIST)

check: fmt-check lint test ## The pre-push gate: the same commands as the CI check job

# CI sets RUSTFLAGS=-D warnings for the whole workflow.
check lint test: export RUSTFLAGS = -D warnings

fmt: ## Format in place
	$(CARGO) fmt --all

fmt-check: ## Check formatting
	$(CARGO) fmt --all --check

lint: ## Clippy, warnings as errors
	$(CARGO) clippy --workspace --all-targets --all-features --locked -- -D warnings

test: ## Run the tests
	$(CARGO) test --workspace --all-features --locked

hooks: ## Install the git hooks (prek: pre-commit and pre-push)
	prek install

include release.mk
