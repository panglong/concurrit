#!/bin/bash

rsync --exclude='glog**' --exclude='gtest**' --exclude='pin/**' -e ssh --numeric-ids --quiet --ignore-errors --archive --delete-during --force potus.cs.berkeley.edu:/home/elmas/concurrit/ $CONCURRIT_HOME/ 