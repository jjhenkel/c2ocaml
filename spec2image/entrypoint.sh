#!/bin/bash

# Want to support 
# build / bash / env all with -c --commit flag 
# Check for (enhanced) get opts

OPTIONS=c:
LONGOPTS=commit:

# Temporarily store output to be able to check for errors
# -e.g. use “--options” parameter by name to activate quoting/enhanced mode
# -pass arguments only via  -- "$@"  to separate them correctly
PARSED=$(getopt --options=$OPTIONS --longoptions=$LONGOPTS --name "$0" -- "$@")
if [[ $? -ne 0 ]]; then
    # e.g. $? == 1
    #  then getopt has complained about wrong arguments to stdout
    exit 2
fi

# Read getopt’s output this way to handle the quoting right:
eval set -- "$PARSED"

# Now enjoy the options in order and nicely split until we see --
while true; do
    case "$1" in
        -c|--commit)
            c="$2"
            shift 2
            ;;
        --)
            shift
            break
            ;;
        *)
            echo "Error: unmatched arg"
            exit 3
            ;;
    esac
done

# Maybe this project wasn't git-based
if [ "$PROJECT_GIT_URL" ]
then
  pushd /target/$PROJECT > /dev/null

  # Get us to the right commit 
  if [ "$c" ]
  then
    git checkout $c
  fi

  # Add the commit hash to our env
  export PROJECT_SHA=$(git rev-parse HEAD)

  popd > /dev/null

  export PROJECT_GIT_BASED="YES"
else
  # Export vars indicating this
  export PROJECT_SHA="N/A"
  export PROJECT_GIT_BASED="NO"
fi

# Handle non-option arguments
if [[ $# -ne 1 ]]; then
    bash
    exit 0
fi

# Pick and option to run against the container
if [ $1 = "bash" ]
then
  bash
  exit 0
elif [ $1 = "build" ]
then
  # Here we actually do the build given in the spec (wrapped in do-make)
  echo "Building $PROJECT..."
  /target/do-make
  STATUS=$?
  if [ $STATUS -ne 0 ]; then
    echo "Failed to build (do-make failed)."
    exit $STATUS
  else
    echo "Build succeeded!"
    exit 0
  fi
elif [ $1 = "env" ]
then 
  # Show the env!
  env
  exit 0
fi
