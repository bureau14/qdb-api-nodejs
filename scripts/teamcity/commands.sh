#!/bin/bash

function nvm_load {
    case "$(uname)" in
        MINGW*)
        ;;
        *)
            export NVM_DIR="$HOME/.nvm"
            [ -s "$NVM_DIR/nvm.sh" ] && \. "$NVM_DIR/nvm.sh"  # This loads nvm
        ;;
    esac
}

function nvm_use {
    echo "Using version: $NODE_VERSION -- arch: $NODE_ARCH";
    nvm_load
    nvm use $NODE_VERSION $NODE_ARCH

    case "$(uname)" in
        MINGW*)
            export NPM=npm
        ;;
        *)
            export NPM="nvm exec $NODE_VERSION npm"
        ;;
    esac
}

function npm_config {    
    nvm_load
    case "$(uname)" in
        MINGW*)
            $NPM config set msvs_version 2017
        ;;
        *)
            $NPM config set python python3
        ;;
    esac
}
