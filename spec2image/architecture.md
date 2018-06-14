# spec2image - architecture

The key idea behind this tool is to work past some problems one can encounter when leveraging Docker as a solution for storing hundreds of projects and their builds. `spec2image` works in the following way:

 1. A _specification file_ (usually ends in .env) is provided and we `source` it.
 ```bash
 # Ex. 
 source ./examples/c/redis.env
 ```
 2. A _build context_ is created that contains the Dockerfile. Any special modifications (things that we _cannot accomplish via `--build-arg`_) happen on the copy of the Dockerfile in the build context. 
 ```bash
  # Make sure context is empty
  rm -rf $DIR/.context-$PROJECT
  mkdir -p $DIR/.context-$PROJECT
  mkdir -p $DIR/.context-$PROJECT/tools

  # Prep specific docker context (to enable better layer caching)
  cp $spec $DIR/.context-$PROJECT
  cp $DIR/Dockerfile $DIR/.context-$PROJECT
  cp $DIR/entrypoint.sh $DIR/.context-$PROJECT
  cp $DIR/tools/* $DIR/.context-$PROJECT/tools
  ```
  3. Environment variables are used to parameterize a call to `docker build`. Each key/value pair in the `source`'d specification file can be passed as a `build arg (--build-arg)` to `docker build` in this manner.
  ```bash
  docker build \
      --label="$l" \
      -t $t/$PROJECT \
      -f $DIR/.context-$PROJECT/Dockerfile \
      --build-arg PROJECT="$PROJECT" \
      --build-arg PROJECT_GIT_URL="$PROJECT_GIT_URL" \
      --build-arg PROJECT_DEPS="$PROJECT_DEPS" \
      --build-arg PROJECT_SETUP="$PROJECT_SETUP" \
      --build-arg PROJECT_CLEAN="$PROJECT_CLEAN" \
      --build-arg PROJECT_MAKE="$PROJECT_MAKE" \
      --build-arg PLUGIN_SPEC="$PLUGIN_SPEC" \
      $DIR/.context-$PROJECT 
  ```
  4. The Dockerfile has a similar section where it explicitly lists the `--build-arg`'s it accepts:
  ```bash
  ARG PROJECT
  ARG PROJECT_GIT_URL
  ARG PROJECT_DEPS
  ARG PROJECT_SETUP
  ARG PROJECT_CLEAN
  ARG PROJECT_MAKE
  ARG PLUGIN_SPEC
  ```  
  5. An *entrypoint* is set. This can be overridden by the user via `docker run --entrypoint=blah`. Note: the entrypoint this tool provides allows for a few basic operations (for convenience) and falls-back to dropping the user into a full interactive session.
  ```bash
  # Copy our entrypoint and set it as the actual entrypoint
  COPY entrypoint.sh .
  ENTRYPOINT ["./entrypoint.sh"]
  ```
  6. The image finishes building and is ready to be used to create containers.