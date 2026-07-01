#!/bin/bash

alias pset="cmake --preset default"
alias pbuild="cmake --build --preset default"
alias prun="cmake --build --preset default --target run"

function pall() {
    pset && pbuild && prun
}
