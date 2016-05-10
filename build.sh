#!/bin/sh

export SONAME=libomxil-nx
export NATIVE_BUILD=yes

name=libomxil-nx
version=`rpmspec --query --srpm --queryformat="%{version}" packaging/${name}.spec`
buildname=${name}-${version}

git archive --format=tar.gz --prefix=$buildname/ -o ~/rpmbuild/SOURCES/$buildname.tar.gz HEAD

rpmbuild -ba packaging/${name}.spec
