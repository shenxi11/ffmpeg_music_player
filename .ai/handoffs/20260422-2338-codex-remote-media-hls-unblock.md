## Goal
Implement the server-side part of the remote local-audio recovery pass under the strict CloudMusic backend workflow: edit only `shen@192.168.1.208:/home/shen/microservice-deploy`, keep `/uploads` on a static direct Range path, and stop `/music/local/playback-info` from synchronously blocking on HLS generation.

## Completed
- Connected to `cloudmusic-vm` and ran the minimal truth check inside `/home/shen/microservice-deploy`.
- Inspected the repo-local build/deploy scripts plus split NGINX config and confirmed two root causes on the server side:
  - `internal/media/handler/playback_info.go` still ran `ffmpeg` synchronously inside `/music/local/playback-info`.
  - `deploy/nginx/nginx.split.conf` still hardcoded legacy aliases like `/home/shen/PycharmProjects/flaskProject/uploads/`.
- Implemented and pushed backend changes only in the VM development repo:
  - `internal/media/handler/media_handler.go`
  - `internal/media/handler/playback_info.go`
  - `scripts/start_split_stack.sh`
  - `deploy/nginx/nginx.split.conf`
- Server behavior after the change:
  - `playback-info` now always returns `direct_url` quickly.
  - HLS is reported only when a manifest is already ready.
  - Missing or broken HLS no longer blocks the request; background HLS generation is scheduled once per cache key.
  - split-mode NGINX aliases for `/uploads`, `/hls`, and `/video` are rendered from the actual deployment root at startup instead of pointing at the old absolute path.
- Validation completed on the VM development repo:
  - `go test ./internal/media/handler`
  - `./build.sh`
  - `bash -n scripts/start_split_stack.sh`
  - `nginx -t -p /home/shen/microservice-deploy/.runtime/nginx/ -c /home/shen/microservice-deploy/.runtime/nginx/nginx.split.conf`
- Commit and push completed from the VM development repo:
  - `c63cb11 fix(media): unblock direct uploads path and async hls readiness`

## Changed Scope
- No backend code was edited in the local Windows checkout.
- No deployment directory was used for development or git commit.
- No remote formal deployment was executed in this pass.

## Open Work
- Client side still needs to complete the route switch and seek recovery simplification so public remote service-local audio prefers direct `/uploads` and no longer loops on recover/degrade.
- Formal deployment still needs to be done from `/home/shen/CloudMusic` or `/root/CloudMusic` after checking those worktrees are clean.
- Runtime verification in the VM development repo is currently blocked before services start because `config.yaml` and `configs/config.yaml` still contain the placeholder DB password `change_me_please`.

## Risks
- Because `microservice-deploy` could not start its split stack with the placeholder DB password, this session did not complete an end-to-end live HTTP playback test from that dev checkout.
- `git status --short` in `/home/shen/microservice-deploy` still shows unrelated untracked items like `media_indexer` and the Chinese-named example directory; they were not touched.
- The local repo task remains `doing` because the paired client-side work and formal deployment are still pending.

## Next Entry Point
- Resume from `shen@192.168.1.208:/home/shen/microservice-deploy`.
- First verify the correct DB credentials for the development checkout, then rerun:
  - `./start_split.sh`
  - `curl -I -H 'Range: bytes=0-1' http://127.0.0.1:8080/uploads/<sample>`
  - `curl 'http://127.0.0.1:8080/music/local/playback-info?music_path=<relative path>'`
- After live validation, deploy from a clean `CloudMusic` worktree by pulling `origin/main`, rebuilding, and restarting in the target deployment directory.
