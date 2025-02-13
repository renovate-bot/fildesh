
You can run the deployed Docker image like this:
```shell
docker run --rm -i ghcr.io/fildesh/fildesh:latest < example/hello.fildesh
```

This isn't well-tested because it's probably not that useful; a containerized script will only find builtin commands.
If it is useful for you and something doesn't work, please file a bug!

A GitHub workflow builds and pushes that `ghcr.io/fildesh/fildesh:latest` image whenever we merge to the `deploy` or `release` branch.
That workflow doesn't use `compose.yaml`, but it makes manual testing easier.

To build & test an image locally, run:
```shell
cd pkg/docker/
bazel build -c opt //...
docker compose build
docker run --rm -i fildesh < ../../example/hello.fildesh
```
