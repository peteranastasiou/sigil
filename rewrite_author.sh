#!/bin/bash

git filter-branch -f --commit-filter '
    if [ "$GIT_AUTHOR_EMAIL" = "peter.a@inovor.com" ];
    then
        GIT_AUTHOR_EMAIL="8896307+peteranastasiou@users.noreply.github.com"
        git commit-tree "$@";
    else
        git commit-tree "$@";
    fi' HEAD

