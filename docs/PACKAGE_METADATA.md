# 📦 AILEE-Core Package Metadata Guide

> **Comprehensive guide to project metadata, versioning, and package management**

---

## 📋 Table of Contents

- [Overview](#overview)
- [Version Information](#version-information)
- [Package Files](#package-files)
- [Metadata Consistency](#metadata-consistency)
- [Keywords & Discoverability](#keywords--discoverability)
- [Dependency Management](#dependency-management)

---

## 🎯 Overview

AILEE-Core uses multiple metadata files to describe the project across different ecosystems:

| File | Purpose | Ecosystem |
|------|---------|-----------|
| **pyproject.toml** | Python package metadata & dependencies | Python/PyPI |
| **CITATION.cff** | Academic citation metadata | Research/Academic |
| **CMakeLists.txt** | C++ build configuration & version | C++/CMake |
| **requirements.txt** | Python dependency pinning | Python (legacy) |

---

## 📌 Version Information

**Current Version: 1.2.1**

### Version Declaration Locations

All version numbers are synchronized across:

1. **CMakeLists.txt** (Line 2):
   ```cmake
   project(AILEE_Core VERSION 1.2.1 LANGUAGES CXX)
   ```

2. **pyproject.toml** (Line 6):
   ```toml
   version = "1.2.1"
   ```

3. **CITATION.cff** (Line 5):
   ```yaml
   version: 1.2.1
   ```

4. **api/config.py** (Line 93-95):
   ```python
   app_version: str = Field(default="1.2.1", ...)
   ```

### When Updating Versions

Update all four locations simultaneously to maintain consistency:

```bash
# 1. Update CMakeLists.txt
sed -i 's/VERSION [0-9.]\+/VERSION X.Y.Z/' CMakeLists.txt

# 2. Update pyproject.toml
sed -i 's/^version = .*/version = "X.Y.Z"/' pyproject.toml

# 3. Update CITATION.cff
sed -i 's/^version: .*/version: X.Y.Z/' CITATION.cff

# 4. Update api/config.py
sed -i 's/default="[0-9.]\+"/default="X.Y.Z"/' api/config.py
```

---

## 📄 Package Files

### pyproject.toml (Primary Python Metadata)

**Purpose**: Modern Python packaging standard (PEP 518, PEP 621)

**Key Sections**:

```toml
[project]
name = "ailee-core"
version = "1.2.1"
description = "Bitcoin-anchored Layer-2 orchestration and verification framework..."
license = {text = "PolyForm Noncommercial 1.0.0"}
authors = [{name = "Don Michael Feeney"}]
keywords = ["bitcoin", "layer-2", "sidechain", ...]

dependencies = [
    "fastapi>=0.104.1,<1.0.0",
    "uvicorn[standard]>=0.24.0,<1.0.0",
    ...
]

[project.optional-dependencies]
dev = ["pytest>=7.4.0", ...]
monitoring = ["prometheus-client>=0.18.0"]
```

**Benefits**:
- ✅ Unified dependency management
- ✅ Version constraints with ranges
- ✅ Optional dependency groups (dev, docs, monitoring)
- ✅ Project metadata in standardized format
- ✅ Tool configuration (black, ruff, mypy, pytest)

### CITATION.cff (Academic Metadata)

**Purpose**: Standardized citation format for research and academic use (Citation File Format v1.2.0)

**Key Fields**:

```yaml
title: "AILEE-Core: Bitcoin-Anchored Layer-2..."
version: 1.2.1
date-released: 2025-01-15
authors:
  - family-names: Feeney
    given-names: Don Michael
abstract: >
  Comprehensive project description...
keywords:
  - Bitcoin
  - Layer-2
  ...
```

**Use Cases**:
- 📚 Academic citations
- 🔍 Research paper references
- 📊 GitHub citation integration
- 🏆 Project recognition

### CMakeLists.txt (C++ Build Configuration)

**Purpose**: C++ project build system and dependency management

**Version Declaration**:
```cmake
project(AILEE_Core VERSION 1.2.1 LANGUAGES CXX)
```

**Manages**:
- C++ dependencies (OpenSSL, CURL, ZeroMQ, etc.)
- Build targets and executables
- Compiler flags and optimizations
- Test configuration

### requirements.txt (Legacy Python Dependencies)

**Purpose**: Backward compatibility for pip-based installations

**Format**:
```
fastapi==0.104.1
uvicorn[standard]==0.24.0
pydantic==2.5.0
...
```

**Status**: Maintained for backward compatibility; `pyproject.toml` is the primary source

---

## 🔄 Metadata Consistency

### Project Title

**Consistent across all files**:

```
AILEE-Core: Bitcoin-Anchored Layer-2 Orchestration and Verification Framework
```

### Project Description

**Core Concept** (appears in all metadata):
> Bitcoin-anchored Layer-2 orchestration and verification framework combining ambient AI-driven task scheduling with deterministic state verification, federated security models, and recovery-first protocol design.

**Variations by context**:

- **CITATION.cff**: Academic focus with detailed technical features
- **pyproject.toml**: Concise package description
- **README.md**: Marketing/user-facing description
- **API config**: Runtime application description

### Author Information

**Consistent across all files**:
- **Name**: Don Michael Feeney
- **GitHub**: @dfeen87
- **Affiliation**: Independent Research
- **License**: PolyForm Noncommercial 1.0.0

---

## 🏷️ Keywords & Discoverability

### Complete Keyword Set

The following keywords are used across metadata files to improve discoverability:

#### Core Technology
- `bitcoin`
- `layer-2` / `Bitcoin L2`
- `sidechain`
- `blockchain`
- `cryptocurrency`
- `blockchain-infrastructure`

#### Architecture & Security
- `federated-security`
- `deterministic-verification`
- `orchestration`
- `distributed-systems`

#### AI & Intelligence
- `ambient-ai`
- `task-scheduling`
- `energy-efficiency`

#### Implementation
- `fastapi`
- `rest-api`
- `python`
- `cpp` / `C++17`

#### Research Focus
- `recovery-protocol`
- `verifiable-computation`

### Usage by File

| Keywords | pyproject.toml | CITATION.cff | README.md |
|----------|:--------------:|:------------:|:---------:|
| Bitcoin-related | ✅ | ✅ | ✅ |
| AI/Orchestration | ✅ | ✅ | ✅ |
| Tech Stack | ✅ | ✅ | ✅ |
| Security Model | ✅ | ✅ | ✅ |

---

## 📦 Dependency Management

### Python Dependencies (pyproject.toml)

**Production Dependencies**:
```toml
dependencies = [
    "fastapi>=0.104.1,<1.0.0",      # REST API framework
    "uvicorn[standard]>=0.24.0",     # ASGI server
    "pydantic>=2.5.0,<3.0.0",        # Data validation
    "pydantic-settings>=2.1.0",      # Settings management
    "python-multipart>=0.0.6",       # Form data parsing
    "slowapi>=0.1.9",                # Rate limiting
    "psutil>=5.9.6",                 # System metrics
    "python-jose[cryptography]>=3.3.0" # JWT tokens
]
```

**Optional Dependencies**:
```toml
[project.optional-dependencies]
dev = ["pytest>=7.4.0", "black>=23.0.0", "ruff>=0.1.0", "mypy>=1.5.0"]
docs = ["mkdocs>=1.5.0", "mkdocs-material>=9.4.0"]
monitoring = ["prometheus-client>=0.18.0"]
```

**Installation**:
```bash
# Production dependencies
pip install -e .

# Development dependencies
pip install -e ".[dev]"

# All dependencies
pip install -e ".[dev,docs,monitoring]"
```

### C++ Dependencies (CMakeLists.txt)

**Required Libraries**:
- **OpenSSL** (≥1.1.0): SHA256, cryptographic operations
- **CURL** (≥7.0): HTTP client for Bitcoin RPC
- **ZeroMQ** (≥4.0): Message queue for Bitcoin ZMQ listener
- **cppzmq**: C++ bindings for ZeroMQ
- **nlohmann/json**: JSON parsing (header-only)
- **cpp-httplib**: HTTP server (header-only)
- **yaml-cpp**: YAML configuration parsing
- **RocksDB** (optional): Persistent storage

**Installation**:
```bash
# Ubuntu/Debian
sudo apt-get install -y build-essential cmake libssl-dev \
    libcurl4-openssl-dev libzmq3-dev libcppzmq-dev \
    libjsoncpp-dev libyaml-cpp-dev librocksdb-dev

# macOS
brew install cmake openssl curl zeromq cppzmq jsoncpp yaml-cpp rocksdb
```

---

## 🚀 Using pyproject.toml

### Building & Installing

```bash
# Install in development mode (editable)
pip install -e .

# Build distribution packages
pip install build
python -m build

# Install from built package
pip install dist/ailee_core-1.2.1-py3-none-any.whl
```

### Running with pyproject.toml

After installation, run the API server with uvicorn:

```bash
# Install the package
pip install -e .

# Run with uvicorn
uvicorn api.main:app --host 0.0.0.0 --port 8080 --reload

# Or use the standard approach from the project root
cd /path/to/AILEE-Protocol-Core-For-Bitcoin
uvicorn api.main:app --host 0.0.0.0 --port 8080
```

### Tool Configuration

The `pyproject.toml` includes configuration for development tools:

- **Black** (code formatter): 120 char line length, Python 3.9-3.12
- **Ruff** (linter): Python 3.9+ compatible
- **MyPy** (type checker): Python 3.9+, strict type checking
- **Pytest** (test runner): Test discovery and configuration

---

## 📊 Metadata Validation

### Check Consistency

Use this script to verify metadata consistency:

```bash
#!/bin/bash
# check-metadata.sh

CMAKE_VERSION=$(grep "project(AILEE_Core VERSION" CMakeLists.txt | grep -oP '\d+\.\d+\.\d+')
PYPROJECT_VERSION=$(grep "^version = " pyproject.toml | grep -oP '\d+\.\d+\.\d+')
CITATION_VERSION=$(grep "^version: " CITATION.cff | grep -oP '\d+\.\d+\.\d+')
API_VERSION=$(grep 'app_version.*default=' api/config.py | grep -oP '\d+\.\d+\.\d+')

echo "CMakeLists.txt: $CMAKE_VERSION"
echo "pyproject.toml: $PYPROJECT_VERSION"
echo "CITATION.cff:   $CITATION_VERSION"
echo "api/config.py:  $API_VERSION"

if [ "$CMAKE_VERSION" = "$PYPROJECT_VERSION" ] && \
   [ "$PYPROJECT_VERSION" = "$CITATION_VERSION" ] && \
   [ "$CITATION_VERSION" = "$API_VERSION" ]; then
    echo "✅ All versions match!"
else
    echo "❌ Version mismatch detected!"
    exit 1
fi
```

### Validate pyproject.toml

```bash
# Install validation tools
pip install validate-pyproject

# Validate syntax
validate-pyproject pyproject.toml

# Check with pip
pip install -e . --dry-run
```

---

## 🔗 References

### Standards

- **PEP 518**: Specifying Minimum Build System Requirements
- **PEP 621**: Storing project metadata in pyproject.toml
- **PEP 517**: A build-system independent format for source trees
- **Citation File Format 1.2.0**: https://citation-file-format.github.io/

### Documentation

- **Python Packaging Guide**: https://packaging.python.org/
- **CMake Documentation**: https://cmake.org/documentation/
- **CITATION.cff Guide**: https://github.com/citation-file-format/citation-file-format

---

## ✅ Best Practices

1. **Always update all version files simultaneously**
2. **Use semantic versioning (MAJOR.MINOR.PATCH)**
3. **Pin production dependencies in requirements.txt**
4. **Use version ranges in pyproject.toml for flexibility**
5. **Update CITATION.cff when adding major features**
6. **Keep keywords synchronized across metadata files**
7. **Validate metadata files before releases**
8. **Document breaking changes in CHANGELOG.md**

---

**Last Updated**: 2025-01-15  
**Version**: 1.2.1  
**Maintainer**: Don Michael Feeney (@dfeen87)
