#!/bin/bash

echo "Building new plugin for Shelldone"
echo
echo -n "name: "
read PLUGINNAME
echo -n "type (PROMPT, PARSING, BUILTIN): "
read PLUGINTYPE

dir=$(tr A-Z a-z <<<$PLUGINTYPE)
name=$(tr A-Z a-z <<<$PLUGINNAME)

[ -d ../plugins/$dir ] || mkdir ../plugins/$dir

cp -r template ../plugins/$dir/$name

mv ../plugins/$dir/$name/template.c ../plugins/$dir/$name/$name.c

sed -i "s/@PLUGINNAME@/$PLUGINNAME/g;s/@PLUGINTYPE@/$PLUGINTYPE/g" \
../plugins/$dir/$name/*

echo "New plugin '$name' generated."
echo "Have fun coding it ;)"
