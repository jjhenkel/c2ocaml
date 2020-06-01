SHELL=/bin/bash

# Cross-platform realpath from 
# https://stackoverflow.com/a/18443300
# NOTE: Adapted for Makefile use
define BASH_FUNC_realpath%%
() {
  OURPWD=$PWD
  cd "$(dirname "$1")"
  LINK=$(readlink "$(basename "$1")")
  while [ "$LINK" ]; do
    cd "$(dirname "$LINK")"
    LINK=$(readlink "$(basename "$1")")
  done
  REALPATH="$PWD/$(basename "$1")"
  cd "$OURPWD"
  echo "$REALPATH"
}
endef
export BASH_FUNC_realpath%%

ROOT_DIR:=$(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))

.PHONY: help
.PHONY: gcc7.3.0
.PHONY: c2ocaml
.PHONY: redis
.PHONY: nginx
.PHONY: hexchat
.PHONY: nmap
.PHONY: curl
.PHONY: rq4
.PHONY: linux

.DEFAULT_GOAL := help

help: ## This help.
	@grep -E \
		'^[\/\.0-9a-zA-Z_-]+:.*?## .*$$' \
		$(MAKEFILE_LIST) \
		| sort \
		| awk 'BEGIN {FS = ":.*?## "}; \
		       {printf "\033[36m%-30s\033[0m %s\n", $$1, $$2}'

gcc7.3.0: ## Ensures that the gcc7.3.0 image is pulled from docker hub and runs our c2ocaml-gcc7.3.0 container. 
	@echo "[c2ocaml] Ensuring we have gcc7.3.0"
	docker pull jjhenkel/gcc7.3.0
	@echo "[c2ocaml] Creating c2ocaml-gcc7.3.0 container"
	docker rm c2ocaml-gcc7.3.0 &> /dev/null || true
	docker run --name=c2ocaml-gcc7.3.0 jjhenkel/gcc7.3.0 /entrypoint.sh

c2ocaml: ## Ensures that our c2ocaml source-to-source transformation plugin is pulled from docker hub and runs our c2ocaml-build container.
	@echo "[c2ocaml] Ensuring we have c2ocaml"
	docker pull jjhenkel/c2ocaml
	@echo "[c2ocaml] Creating c2ocaml-build container"
	docker rm c2ocaml-build &> /dev/null || true
	docker run --name=c2ocaml-build --volumes-from=c2ocaml-gcc7.3.0:ro -v ${ROOT_DIR}/plugin:/app/Lets/Transform/plugin jjhenkel/c2ocaml

redis: gcc7.3.0 c2ocaml ## Builds redis and transforms built files from C/C++ to OCaml.
	@echo "[c2ocaml] Building redis docker image..."
	${ROOT_DIR}/spec2image/spec2image -e ${ROOT_DIR}/corpus/entrypoint.sh -l c2ocaml -t c2ocaml ${ROOT_DIR}/corpus/redis.env
	@echo "[c2ocaml] Built!"
	@echo "[c2ocaml] Ingesting redis..."
	docker run -it --rm \
		--volumes-from=c2ocaml-gcc7.3.0 \
		--volumes-from=c2ocaml-build \
		-v ${ROOT_DIR}/artifacts/redis:/common/facts \
		c2ocaml/redis \
		40d5df6547308db2f5d71432b10fa84a9844edff
	@echo "[c2ocaml] Ingested $$(find ${ROOT_DIR}/artifacts/redis -type f -name "*.ml" | wc -l) procedures!"
	@echo "[c2ocaml] Merging ingested procedures..."
	${ROOT_DIR}/merge-sources ${ROOT_DIR}/artifacts/redis
	@echo "[c2ocaml] Created $$(find ${ROOT_DIR}/artifacts/redis-merged -type f | wc -l) merged files"
	@echo "[c2ocaml] Cleaning up..."
	docker run -it --rm -v ${ROOT_DIR}/artifacts:/common/facts debian:stretch rm -rf /common/facts/redis
	mv ${ROOT_DIR}/artifacts/redis-merged ${ROOT_DIR}/artifacts/redis
	@echo "[c2ocaml] Finished! Artifacts placed in the "artifacts/redis" directory."
	@echo "[c2ocaml] Use lsee to generate traces"

nginx: gcc7.3.0 c2ocaml ## Builds nginx and transforms built files from C/C++ to OCaml.
	@echo "[c2ocaml] Building nginx docker image..."
	${ROOT_DIR}/spec2image/spec2image -e ${ROOT_DIR}/corpus/entrypoint.sh -l c2ocaml -t c2ocaml ${ROOT_DIR}/corpus/nginx.env
	@echo "[c2ocaml] Built!"
	@echo "[c2ocaml] Ingesting nginx..."
	docker run -it --rm \
		--volumes-from=c2ocaml-gcc7.3.0 \
		--volumes-from=c2ocaml-build \
		-v ${ROOT_DIR}/artifacts/nginx:/common/facts \
		c2ocaml/nginx \
		4bf4650f2f10f7bbacfe7a33da744f18951d416d
	@echo "[c2ocaml] Ingested $$(find ${ROOT_DIR}/artifacts/nginx -type f -name "*.ml" | wc -l) procedures!"
	@echo "[c2ocaml] Merging ingested procedures..."
	${ROOT_DIR}/merge-sources ${ROOT_DIR}/artifacts/nginx
	@echo "[c2ocaml] Created $$(find ${ROOT_DIR}/artifacts/nginx-merged -type f | wc -l) merged files"
	@echo "[c2ocaml] Cleaning up..."
	docker run -it --rm -v ${ROOT_DIR}/artifacts:/common/facts debian:stretch rm -rf /common/facts/nginx
	mv ${ROOT_DIR}/artifacts/nginx-merged ${ROOT_DIR}/artifacts/nginx
	@echo "[c2ocaml] Finished! Artifacts placed in the "artifacts/nginx" directory."
	@echo "[c2ocaml] Use lsee to generate traces"

hexchat: gcc7.3.0 c2ocaml ## Builds hexchat and transforms built files from C/C++ to OCaml.
	@echo "[c2ocaml] Building hexchat docker image..."
	${ROOT_DIR}/spec2image/spec2image -e ${ROOT_DIR}/corpus/entrypoint.sh -l c2ocaml -t c2ocaml ${ROOT_DIR}/corpus/hexchat.env
	@echo "[c2ocaml] Built!"
	@echo "[c2ocaml] Ingesting hexchat..."
	docker run -it --rm \
		--volumes-from=c2ocaml-gcc7.3.0 \
		--volumes-from=c2ocaml-build \
		-v ${ROOT_DIR}/artifacts/hexchat:/common/facts \
		c2ocaml/hexchat \
		a3db4e577307742965f5ba75daf03146164bd211
	@echo "[c2ocaml] Ingested $$(find ${ROOT_DIR}/artifacts/hexchat -type f -name "*.ml" | wc -l) procedures!"
	@echo "[c2ocaml] Merging ingested procedures..."
	${ROOT_DIR}/merge-sources ${ROOT_DIR}/artifacts/hexchat
	@echo "[c2ocaml] Created $$(find ${ROOT_DIR}/artifacts/hexchat-merged -type f | wc -l) merged files"
	@echo "[c2ocaml] Cleaning up..."
	docker run -it --rm -v ${ROOT_DIR}/artifacts:/common/facts debian:stretch rm -rf /common/facts/hexchat
	mv ${ROOT_DIR}/artifacts/hexchat-merged ${ROOT_DIR}/artifacts/hexchat
	@echo "[c2ocaml] Finished! Artifacts placed in the "artifacts/hexchat" directory."
	@echo "[c2ocaml] Use lsee to generate traces"

nmap: gcc7.3.0 c2ocaml ## Builds nmap and transforms built files from C/C++ to OCaml.
	@echo "[c2ocaml] Building nmap docker image..."
	${ROOT_DIR}/spec2image/spec2image -e ${ROOT_DIR}/corpus/entrypoint.sh -l c2ocaml -t c2ocaml ${ROOT_DIR}/corpus/nmap.env
	@echo "[c2ocaml] Built!"
	@echo "[c2ocaml] Ingesting nmap..."
	docker run -it --rm \
		--volumes-from=c2ocaml-gcc7.3.0 \
		--volumes-from=c2ocaml-build \
		-v ${ROOT_DIR}/artifacts/nmap:/common/facts \
		c2ocaml/nmap \
		88b68c45aacc29639940023d9574dc2e851bf8ab
	@echo "[c2ocaml] Ingested $$(find ${ROOT_DIR}/artifacts/nmap -type f -name "*.ml" | wc -l) procedures!"
	@echo "[c2ocaml] Merging ingested procedures..."
	${ROOT_DIR}/merge-sources ${ROOT_DIR}/artifacts/nmap
	@echo "[c2ocaml] Created $$(find ${ROOT_DIR}/artifacts/nmap-merged -type f | wc -l) merged files"
	@echo "[c2ocaml] Cleaning up..."
	docker run -it --rm -v ${ROOT_DIR}/artifacts:/common/facts debian:stretch rm -rf /common/facts/nmap
	mv ${ROOT_DIR}/artifacts/nmap-merged ${ROOT_DIR}/artifacts/nmap
	@echo "[c2ocaml] Finished! Artifacts placed in the "artifacts/nmap" directory."
	@echo "[c2ocaml] Use lsee to generate traces"

curl: gcc7.3.0 c2ocaml ## Builds curl and transforms built files from C/C++ to OCaml.
	@echo "[c2ocaml] Building curl docker image..."
	${ROOT_DIR}/spec2image/spec2image -e ${ROOT_DIR}/corpus/entrypoint.sh -l c2ocaml -t c2ocaml ${ROOT_DIR}/corpus/curl.env
	@echo "[c2ocaml] Built!"
	@echo "[c2ocaml] Ingesting curl..."
	docker run -it --rm \
		--volumes-from=c2ocaml-gcc7.3.0 \
		--volumes-from=c2ocaml-build \
		-v ${ROOT_DIR}/artifacts/curl:/common/facts \
		c2ocaml/curl \
		cf448436facd28da1bafe031d14a8bc4f165ddaa
	@echo "[c2ocaml] Ingested $$(find ${ROOT_DIR}/artifacts/curl -type f -name "*.ml" | wc -l) procedures!"
	@echo "[c2ocaml] Merging ingested procedures..."
	${ROOT_DIR}/merge-sources ${ROOT_DIR}/artifacts/curl
	@echo "[c2ocaml] Created $$(find ${ROOT_DIR}/artifacts/curl-merged -type f | wc -l) merged files"
	@echo "[c2ocaml] Cleaning up..."
	docker run -it --rm -v ${ROOT_DIR}/artifacts:/common/facts debian:stretch rm -rf /common/facts/curl
	mv ${ROOT_DIR}/artifacts/curl-merged ${ROOT_DIR}/artifacts/curl
	@echo "[c2ocaml] Finished! Artifacts placed in the "artifacts/curl" directory."
	@echo "[c2ocaml] Use lsee to generate traces"

rq4: gcc7.3.0 c2ocaml ## Builds rq4 and transforms built files from C/C++ to OCaml.
	@echo "[c2ocaml] Building rq4 docker image..."
	${ROOT_DIR}/spec2image/spec2image -e ${ROOT_DIR}/corpus/entrypoint.sh -l c2ocaml -t c2ocaml ${ROOT_DIR}/corpus/changed-error-codes.env
	@echo "[c2ocaml] Built!"
	@echo "[c2ocaml] Ingesting rq4..."
	docker run -it --rm \
		--volumes-from=c2ocaml-gcc7.3.0 \
		--volumes-from=c2ocaml-build \
		-v ${ROOT_DIR}/artifacts/rq4:/common/facts \
		c2ocaml/changed-error-codes
	@echo "[c2ocaml] Ingested $$(find ${ROOT_DIR}/artifacts/rq4 -type f -name "*.ml" | wc -l) procedures!"
	@echo "[c2ocaml] Merging ingested procedures..."
	${ROOT_DIR}/merge-sources ${ROOT_DIR}/artifacts/rq4
	@echo "[c2ocaml] Created $$(find ${ROOT_DIR}/artifacts/rq4-merged -type f | wc -l) merged files"
	@echo "[c2ocaml] Cleaning up..."
	docker run -it --rm -v ${ROOT_DIR}/artifacts:/common/facts debian:stretch rm -rf /common/facts/rq4
	mv ${ROOT_DIR}/artifacts/rq4-merged ${ROOT_DIR}/artifacts/rq4
	@echo "[c2ocaml] Finished! Artifacts placed in the "artifacts/rq4" directory."
	@echo "[c2ocaml] Use lsee to generate traces"

linux: gcc7.3.0 c2ocaml ## Builds linux v4.5-rc4 and transforms built files from C/C++ to OCaml.
	@echo "[c2ocaml] Building linux docker image..."
	${ROOT_DIR}/spec2image/spec2image -e ${ROOT_DIR}/corpus/entrypoint.sh -l c2ocaml -t c2ocaml ${ROOT_DIR}/corpus/linux/v4.5-rc4/allyes.env
	@echo "[c2ocaml] Built!"
	@echo "[c2ocaml] Ingesting linux..."
	docker run -it --rm \
		--volumes-from=c2ocaml-gcc7.3.0 \
		--volumes-from=c2ocaml-build \
		-v ${ROOT_DIR}/artifacts/linux:/common/facts \
		c2ocaml/allyes \
		fd7cd061adcf5f7503515ba52b6a724642a839c8
	@echo "[c2ocaml] Ingested $$(find ${ROOT_DIR}/artifacts/linux -type f -name "*.ml" | wc -l) procedures!"
	@echo "[c2ocaml] Merging ingested procedures..."
	${ROOT_DIR}/merge-sources ${ROOT_DIR}/artifacts/linux
	@echo "[c2ocaml] Created $$(find ${ROOT_DIR}/artifacts/linux-merged -type f | wc -l) merged files"
	@echo "[c2ocaml] Cleaning up..."
	docker run -it --rm -v ${ROOT_DIR}/artifacts:/common/facts debian:stretch rm -rf /common/facts/linux
	mv ${ROOT_DIR}/artifacts/linux-merged ${ROOT_DIR}/artifacts/linux
	@echo "[c2ocaml] Finished! Artifacts placed in the "artifacts/linux" directory."
	@echo "[c2ocaml] Use lsee to generate traces"
