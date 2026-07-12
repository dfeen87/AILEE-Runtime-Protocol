# ⚠️ CRITICAL SECURITY NOTICE - PUBLIC REPOSITORY ⚠️

## This is a PUBLIC Repository

**IMPORTANT:** This repository is **publicly accessible** on GitHub. Anyone on the internet can view all files, commits, and history.

## NEVER Commit These Files

### ❌ Absolutely FORBIDDEN

The following must **NEVER** be committed to this repository:

1. **Real Credentials**
   - Bitcoin RPC usernames/passwords
   - API keys or tokens
   - JWT secrets
   - Database passwords
   - Cloud provider credentials (AWS keys, etc.)

2. **Private Keys**
   - Federation signer private keys (`.key`, `.pem`)
   - TLS/SSL private keys
   - SSH private keys
   - Any cryptographic private key material

3. **Certificates with Private Keys**
   - PKCS#12 files (`.p12`, `.pfx`)
   - Combined certificate+key files
   - HSM key backups

4. **Configuration with Secrets**
   - `.env` files (use `.env.example` only)
   - `secrets.yaml` files (use `secrets.example.yaml` only)
   - Production configuration files

5. **Backup Files**
   - Database dumps
   - Key backup files
   - Encrypted backups with weak passwords

## ✅ Safe to Commit

These are **safe** for public repositories:

1. **Example Files**
   - `.env.example` (with placeholder values only)
   - `secrets.example.yaml` (with `${VARIABLE}` references)
   - Documentation with example configurations

2. **Public Keys**
   - TLS/SSL certificate files (`.crt`, `.pem` without private keys)
   - Public keys for verification

3. **Scripts and Code**
   - Setup scripts (without embedded secrets)
   - Source code
   - Documentation

4. **Public Configuration**
   - Default settings
   - Structure/schema examples

## How to Protect Against Accidental Commits

### 1. Use .gitignore

The `.gitignore` file is configured to exclude common secret files:

```bash
# Already protected:
.env
*.key
*.pem
secrets.yaml
# ... and more
```

### 2. Install Pre-Commit Hooks

```bash
# Install pre-commit hook to detect secrets
cd /opt/ailee-core
./scripts/security/install-git-hooks.sh
```

This will scan commits for potential secrets before they're committed.

### 3. Use Environment Variables

**Never hardcode secrets:**

```bash
# ❌ WRONG - Don't do this!
BITCOIN_RPC_PASSWORD="mySecretPassword123"

# ✅ CORRECT - Use environment variables
BITCOIN_RPC_PASSWORD="${BITCOIN_RPC_PASSWORD}"
```

### 4. Use Secrets Managers

For production:
- **HashiCorp Vault**
- **AWS Secrets Manager**
- **Azure Key Vault**
- **Google Cloud Secret Manager**

### 5. Check Before Committing

Before every commit, verify:

```bash
# Check what you're committing
git diff --cached

# Search for potential secrets
git diff --cached | grep -i "password\|secret\|key\|token"

# If you find secrets, unstage them:
git reset HEAD <file>
```

## If You Accidentally Commit a Secret

### 🚨 IMMEDIATE ACTION REQUIRED

1. **STOP** - Don't push if you haven't already
2. **Remove from history immediately:**

```bash
# If you haven't pushed yet:
git reset --soft HEAD~1
git reset HEAD <file-with-secret>
# Edit file to remove secret
git add <file-with-secret>
git commit

# If you've already pushed (EMERGENCY):
# Contact repository admin immediately
# The secret is compromised and must be rotated
```

3. **Rotate the compromised secret:**
   - Change passwords
   - Regenerate API keys
   - Generate new certificates
   - Update all systems using the secret

4. **Never** rely on deleting a file to remove it from history
   - Git history is permanent
   - Use `git filter-branch` or `BFG Repo-Cleaner` if needed
   - **Assume the secret is compromised once committed**

## Automated Secret Scanning

### GitHub Secret Scanning

GitHub automatically scans public repositories for known secret patterns:
- API keys from major providers
- Private keys
- Database connection strings

**If GitHub detects a secret:**
1. You'll receive an email alert
2. The secret provider may be notified
3. Your secret may be automatically revoked
4. **Rotate the secret immediately**

### Install Local Secret Scanning

```bash
# Install gitleaks (secret scanner)
brew install gitleaks  # macOS
# or
go install github.com/zricethezav/gitleaks/v8@latest  # Linux

# Scan repository
gitleaks detect --source /opt/ailee-core --verbose

# Scan before commit (recommended)
gitleaks protect --staged
```

## Example: Safe Configuration Template

### ❌ WRONG - Never Do This

```yaml
# secrets.yaml - DON'T COMMIT THIS!
bitcoin_rpc:
  username: bitcoinrpc
  password: MyS3cr3tP@ssw0rd  # ← SECRET EXPOSED!
```

### ✅ CORRECT - Safe for Public Repo

```yaml
# secrets.example.yaml - Safe template
bitcoin_rpc:
  username: "${BITCOIN_RPC_USER}"  # ← References environment variable
  password: "${BITCOIN_RPC_PASSWORD}"  # ← Not the actual secret
```

## Reporting Security Issues

If you discover committed secrets in this repository:

1. **DO NOT** post in public issues
2. **Email:** security@ailee.example.com (replace with actual contact)
3. **Include:**
   - Commit hash
   - File path
   - Type of secret
   - Suggested remediation

## Best Practices Checklist

Before committing, verify:

- [ ] No `.env` files (only `.env.example`)
- [ ] No `secrets.yaml` files (only `secrets.example.yaml`)
- [ ] No private keys (`.key`, `.pem`, etc.)
- [ ] No real passwords or API keys
- [ ] No connection strings with credentials
- [ ] All example files use placeholders (`${VARIABLE}`)
- [ ] No files ignored by `.gitignore` being added
- [ ] Ran secret scanner (gitleaks, truffleHog)
- [ ] Reviewed `git diff --cached` for sensitive data

## Educational Purpose

This repository contains:
- ✅ Documentation and guides
- ✅ Example configurations (with placeholders)
- ✅ Security best practices
- ✅ Setup scripts (without embedded secrets)
- ✅ Source code

This repository does NOT contain:
- ❌ Real credentials or secrets
- ❌ Private keys
- ❌ Production configuration
- ❌ Customer data
- ❌ Proprietary information

## Additional Resources

- [GitHub Secret Scanning](https://docs.github.com/en/code-security/secret-scanning)
- [OWASP Secrets Management Cheat Sheet](https://cheatsheetseries.owasp.org/cheatsheets/Secrets_Management_Cheat_Sheet.html)
- [Git Tools - Rewriting History](https://git-scm.com/book/en/v2/Git-Tools-Rewriting-History)
- [BFG Repo-Cleaner](https://rtyley.github.io/bfg-repo-cleaner/)

---

**Remember:** Once committed to a public repository, consider any secret **permanently compromised** and rotate it immediately.

**Last Updated:** 2026-02-15  
**Review:** Before every commit
