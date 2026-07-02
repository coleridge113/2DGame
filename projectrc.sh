#!/bin/bash

alias pset="cmake --preset default"
alias pbuild="cmake --build --preset default"
alias prun="cmake --build --preset default --target run"

function pclean() {
    echo "Cleaning up build..."
    rm -rf build/
}

function pall() {
    pclean && pset && pbuild && prun
}

