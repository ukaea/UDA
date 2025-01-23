# Contributing to UDA

UDA is an open-source project and we welcome contributions. This guide explains the process and expectations for getting your changes added.

---

## Table of Contents

1. [Getting Started](#getting-started)
   - [Fork the Repository](#fork-the-repository)
   - [Create an Issue](#create-an-issue)
   - [Set Up Your Local Environment](#set-up-your-local-environment)
2. [Branching Strategy](#branching-strategy)
3. [Making Changes](#making-changes)
4. [Submitting Your Contribution](#submitting-your-contribution)
5. [Review Process](#review-process)

---

## Getting Started

### 1. Fork the Repository
1. Navigate to the [uda repository](https://github.com/ukaea/uda).
2. Click the **Fork** button in the top-right corner to create a copy of the repository under your GitHub account.

### 2. Create an Issue
Before starting work, please open an issue to describe your contribution. This helps us track ongoing efforts.

1. Go to the [Issues tab](https://github.com/ukaea/uda/issues).
2. Create a new issue with a clear description of the bug, feature, or improvement you want to work on.
3. Wait for a project maintainer to approve or provide feedback.

### 3. Set Up Your Local Environment
See the [development guide](https://ukaea.github.io/UDA/development/) in the UDA documentation pages for more detialed instructions on getting started.

---

## Branching Strategy

We use the **GitFlow branching model**. Here's how branches are structured:

- **`main`**: Contains stable, production-ready code. Do not merge changes directly into `main`.
- **`develop`**: The active development branch. Your contributions should be merged into `develop`.
- **Feature branches**: Used for individual features. Create a feature branch off `develop`:
  ```bash
  git checkout -b feature/your-feature-name develop
  ```

---

## Making Changes

1. Pull the latest changes from the `develop` branch to ensure your branch is up to date:
   ```bash
   git checkout develop
   git pull upstream develop
   ```
2. Create a new feature branch for your work:
   ```bash
   git checkout -b feature/your-feature-name develop
   ```
3. Make your changes and commit them with clear, descriptive commit messages:
   ```bash
   git add .
   git commit -m "Add feature X to improve Y"
   ```

---

## Submitting Your Contribution

1. Push your changes to your fork:
   ```bash
   git push origin feature/your-feature-name
   ```
2. Open a pull request (PR) from your feature branch to the `develop` branch in the original repository:
   - Navigate to your fork on GitHub.
   - Click the **Compare & Pull Request** button.
   - Select the `develop` branch of the original repository as the base branch.
3. Provide a clear title and description for your PR. Reference the issue you are addressing.

---

## Review Process

1. Once your PR is submitted, a maintainer will review it.
2. Be prepared to make changes based on feedback.
3. When your PR is approved, a maintainer will merge it into the `develop` branch.

---

