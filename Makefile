SHELL=/bin/bash
I_PREFIX=ubi

ROOT_DIR:=$(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))

.PHONY: help
.PHONY: gcc7.2.0
.PHONY: c2ocaml
.PHONY: redis

.DEFAULT_GOAL := help

help: ## This help.
	@grep -E \
		'^[\/\.0-9a-zA-Z_-]+:.*?## .*$$' \
		$(MAKEFILE_LIST) \
		| sort \
		| awk 'BEGIN {FS = ":.*?## "}; \
		       {printf "\033[36m%-30s\033[0m %s\n", $$1, $$2}'

gcc7.2.0: ## Ensures that the gcc7.2.0 image is pulled from docker hub and runs our c2ocaml-gcc7.2.0 container. 
	@echo "[c2ocaml] Ensuring we have gcc7.2.0"
	docker pull jjhenkel/gcc7.2.0
	@echo "[c2ocaml] Creating c2ocaml-gcc7.2.0 container"
	docker rm c2ocaml-gcc7.2.0 &> /dev/null || true
	docker run --name=c2ocaml-gcc7.2.0 jjhenkel/gcc7.2.0 /entrypoint.sh

c2ocaml: ## Ensures that our c2ocaml source-to-source transformation plugin is pulled from docker hub and runs our c2ocaml-build container.
	@echo "[c2ocaml] Ensuring we have c2ocaml"
	docker pull jjhenkel/c2ocaml
	@echo "[c2ocaml] Creating c2ocaml-build container"
	docker rm c2ocaml-build &> /dev/null || true
	docker run --name=c2ocaml-build --volumes-from=c2ocaml-gcc7.2.0:ro -v ${ROOT_DIR}/plugin:/app/Lets/Transform/plugin jjhenkel/c2ocaml

redis: gcc7.2.0 c2ocaml ## Builds redis and transforms built files from C/C++ to OCaml.
	@echo "[c2ocaml] Building redis docker image..."
	${ROOT_DIR}/spec2image/spec2image --entrypoint=${ROOT_DIR}/corpus/entrypoint.sh -l c2ocaml -t c2ocaml ${ROOT_DIR}/corpus/redis.env
	@echo "[c2ocaml] Built!"
	@echo "[c2ocaml] Ingesting redis..."
	docker run -it --rm \
		--volumes-from=c2ocaml-gcc7.2.0 \
		--volumes-from=c2ocaml-build \
		-v ${ROOT_DIR}/artifacts/redis:/common/facts \
		c2ocaml/redis \
		40d5df6547308db2f5d71432b10fa84a9844edff
