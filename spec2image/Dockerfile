FROM debian:stretch
# ^ NOTE: first line has to be the FROM
# for our (simple) base logic to work

# (the logic just replace the first line 
# when PROJECT_BASE is set)

# Just layer some basic deps we'd like to have
# always avaliable 
RUN set -ex; apt-get update && apt-get install -y \
  git \
  gcc \
  g++ \
  libc-dev \
  binutils \
  libmpc-dev \
  libgmp-dev \
  libmpfr-dev \
  ca-certificates

# These are the 'parameters' to this image
ARG PROJECT
ARG PROJECT_GIT_URL
ARG PROJECT_DEPS
ARG PROJECT_SETUP
ARG PROJECT_CLEAN
ARG PROJECT_MAKE
ARG PLUGIN_SPEC

# Grab tools and place them into the image
COPY ./tools /common/tools

# Run our setup in one layer
RUN set -ex; \
    apt-get update && apt-get install -y $PROJECT_DEPS; \
    /common/tools/analysis-setup "$PLUGIN_SPEC"; \
    mkdir -p /target/$PROJECT; \
    if [ "${PROJECT_GIT_URL}" ]; then \
      git clone $PROJECT_GIT_URL /target/$PROJECT; \
    fi

# Actually, some things (like Linux) deserve 
# two separate layers to improve caching 
RUN set -ex; cd /target/$PROJECT; \
    if [ "${PROJECT_SETUP}" ]; then \
      eval $PROJECT_SETUP; \
    fi; \
    echo "\
      pushd /target/$PROJECT ;\
      $PROJECT_CLEAN ;\
      STATUS=\$? ;\
      popd ;\ 
      exit \$STATUS\
    " > "/target/do-clean"; \
    chmod +x "/target/do-clean"; \
    echo "\
      pushd /target/$PROJECT ;\
      $PROJECT_MAKE ;\
      STATUS=\$? ;\
      popd ;\
      exit \$STATUS\
    " > "/target/do-make"; \
    chmod +x "/target/do-make"

# Copy our entrypoint and set it as the actual entrypoint
COPY entrypoint.sh .
ENTRYPOINT ["./entrypoint.sh"]
