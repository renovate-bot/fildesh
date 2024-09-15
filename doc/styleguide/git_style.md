# Git Style Guide

## Commit Message
The subject line of a commit messages should have the form `type(scope): description`, with an exclamation mark before the colon if the change will break something.
This is like [Conventional Commits](http://conventionalcommits.org) but with different types.
The following example subject lines demonstrate **all valid commit types** and some common scopes.
```text
cleanup(include)!: deprecated types with _t suffix
cleanup(WORKSPACE.bazel)!: in favor of bzlmod

feat(bazel): cmptxt_test() macro
feat(builtin): cmptxt
feat(lib): to write string literal to FildeshO
feat(sxpb): Support multiline string values

move(bazel public macros)!: to their own files
move(fildesh_lgsize_t use): as Fildesh_lgsize
move(FildeshX|FildeshO methods): to src/lib/xo/
move(Filename type)!: as Filepath

qual(builtin cmptxt): Continue past empty lines
qual(cmake): Provide Fildesh_EXECUTABLE variable
qual(command): Print line number of failed command
qual(syntax): Parse bad string def without leak
qual(test): //test/src:chatty_test on Alpine Linux
qual(test): Fix version test on Windows

tidy(bazel): more with buildifier
tidy(test): s/zec/splice/ and remove shebangs

update(bazel_dep rules_cc): v0.0.1
update(github actions): for freshness
```

## Squash to Stage
Before fast-forwarding to the "trunk" branch, make a squash merge to the "stage" branch, push, and wait for tests to pass.
```shell
# Work on a new branch.
git checkout -b mybranch
# ... do work ...
git commit -a
git push origin mybranch
# ... do more work until ready to merge ...

# Merge from stage.
git merge --no-ff --no-commit stage
git commit
git push origin mybranch

# Merge to stage.
git checkout stage
git merge --squash mybranch
# Sign your commit.
git commit -S
git push origin stage
# ... wait for tests to pass ...

# Merge to trunk.
git checkout trunk
git merge --ff-only stage
git push origin trunk

# Delete the branch.
git branch -D mybranch
git push origin --delete mybranch
```
