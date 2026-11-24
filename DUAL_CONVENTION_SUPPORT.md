# Configuration: Dual Convention Support (v1.0.0)

## Overview

**gitsync v1.0.0 supports both PascalCase (v0.6.0) and snake_case (v1.0+) field names simultaneously.**

This means:
- ✅ Existing configs work unchanged
- ✅ No migration required
- ✅ No breaking changes
- ✅ Users can optionally migrate at their own pace

---

## Supported Field Names

| Feature | PascalCase (v0.6.0) | snake_case (v1.0+) | Both Work? |
|---------|---|---|---|
| Logging verbosity | `LogLevel` | `log_level` | ✅ Yes |
| Source git remote | `SourceRemoteName` | `source_remote_name` | ✅ Yes |
| Branch name template | `RemoteBranchTemplate` | `remote_branch_template` | ✅ Yes |
| Mirror remote URLs | `RemoteUrls` | `remote_urls` | ✅ Yes |
| SSH key types | `SshKeyTypes` | `ssh_key_types` | ✅ Yes |
| Retry attempts | — | `max_retries` | ✅ New field |
| Include branches | — | `include_branches` | ✅ New field |
| Exclude branches | — | `exclude_branches` | ✅ New field |
| Include remotes | — | `include_remotes` | ✅ New field |
| Exclude remotes | — | `exclude_remotes` | ✅ New field |

---

## Examples

### Option A: Existing Config (No Changes)

Your current v0.6.0 config works unchanged in v1.0.0:

```json
{
  "LogLevel": "info",
  "SourceRemoteName": "origin",
  "RemoteBranchTemplate": "%owner%/%reponame%/%branchname%",
  "RemoteUrls": {
    "gitlab": "git@gitlab.com:user/backup.git",
    "github": "git@github.com:user/mirror.git"
  },
  "SshKeyTypes": "ed25519,rsa"
}
```

✅ **Status:** Works perfectly — no changes required

---

### Option B: New Config (Optional snake_case)

Create new configs using cleaner snake_case convention:

```json
{
  "log_level": "info",
  "source_remote_name": "origin",
  "remote_branch_template": "%owner%/%reponame%/%branchname%",
  "ssh_key_types": "ed25519,rsa",
  "remote_urls": {
    "gitlab": "git@gitlab.com:user/backup.git",
    "github": "git@github.com:user/mirror.git"
  },
  "max_retries": 1,
  "exclude_branches": ["main", "master"],
  "exclude_remotes": ["upstream", "base"]
}
```

✅ **Status:** Optional — adopt when you're ready

---

### Option C: Gradual Migration (Mixed)

You can mix old and new conventions in the same config:

```json
{
  "LogLevel": "info",           // ← Old convention (still works)
  "source_remote_name": "origin",  // ← New convention (optional)
  "RemoteUrls": {               // ← Old convention (still works)
    "gitlab": "git@gitlab.com:user/backup.git"
  },
  "max_retries": 1,             // ← New feature (snake_case only)
  "exclude_branches": ["main"]  // ← New feature (snake_case only)
}
```

✅ **Status:** Supported — migrate at your pace

---

### Option D: YAML Format (New)

You can also use YAML with either convention:

```yaml
# Old convention in YAML
LogLevel: info
RemoteUrls:
  gitlab: git@gitlab.com:user/backup.git

# Or new convention
log_level: debug
remote_urls:
  github: git@github.com:user/mirror.git
max_retries: 2
exclude_branches:
  - main
  - master
```

✅ **Status:** Both PascalCase and snake_case work in YAML

---

## Precedence

**If both naming conventions are present:**

Snake_case takes precedence over PascalCase:

```json
{
  "LogLevel": "warn",        // ← This is ignored
  "log_level": "info"        // ← This is used
}
```

Result: `log_level = "info"`

---

## Migration Path (Optional)

You don't have to migrate. But if you want to:

**Step 1:** Test your existing config in v1.0.0 (should work as-is)

**Step 2:** Gradually add new fields to your config:
```json
{
  "LogLevel": "info",           // Keep old for now
  "RemoteUrls": { ... },        // Keep old for now
  "max_retries": 1,             // Add new features
  "exclude_branches": ["main"]
}
```

**Step 3:** When ready, switch to snake_case:
```json
{
  "log_level": "info",          // Switch to new convention
  "remote_urls": { ... },       // Switch to new convention
  "max_retries": 1,
  "exclude_branches": ["main"]
}
```

**Step 4:** No step 4 — you're done! ✅

---

## Configuration Hierarchy (All Layers Support Both)

Each layer (global → org → repo) can use either convention:

```
~/.gitsync/config.json
{
  "LogLevel": "info"            // PascalCase OK
}
        ↓ (merge)
../Company/.gitsync/config.json
{
  "log_level": "debug",         // snake_case OK
  "remote_urls": { ... }
}
        ↓ (merge)
repo/.gitsync/config.json
{
  "LogLevel": "trace",          // Can mix!
  "exclude_branches": ["wip/*"]
}
        ↓ (result)
Final merged config uses both conventions
```

✅ All combinations work

---

## Q&A

**Q: Do I have to migrate my config to v1.0.0?**  
A: No. Your current config works unchanged.

**Q: Will old configs break in v1.0.0?**  
A: No. Old PascalCase configs are fully supported.

**Q: When should I use snake_case?**  
A: When creating new configs or gradually updating existing ones. It's the idiomatic C++ convention.

**Q: What if I have both field names in the same config?**  
A: snake_case takes precedence. The old field is ignored. This lets you migrate gradually.

**Q: Does this affect other features (filtering, retry, etc.)?**  
A: No. Those features are added alongside dual-convention support.

**Q: What about YAML vs JSON?**  
A: Both file formats work with both naming conventions. Mix and match as needed.

---

## Summary

| Aspect | v0.6.0 Config | v1.0.0 Behavior |
|--------|---|---|
| PascalCase fields | ✅ Supported | ✅ Still supported |
| snake_case fields | ❌ N/A | ✅ Newly supported |
| New features | ❌ N/A | ✅ Optional fields |
| JSON files | ✅ Supported | ✅ Still supported |
| YAML files | ❌ Not supported | ✅ Newly supported |
| Migration required | N/A | ❌ No |
| Breaking changes | N/A | ❌ No |

---

**Result:** Perfect backward compatibility + cleaner new convention + zero forced migration. 🎉

