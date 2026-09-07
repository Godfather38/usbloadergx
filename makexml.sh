#! /bin/bash

# Semantic versioning would be better, but for now we'll continue to use the revision
rev_new=$(cat version.txt)
rev_old=$(sed -n 2p ./source/version.h 2>/dev/null | cut -d '"' -f 2)

# Retrieve git info
git_new=$(git rev-parse HEAD 2>/dev/null | head -c 7)
[ -z "$git_new" ] && git_new="0000001"
commit_message=$(git show -s --format="%<(52,trunc)%s" HEAD 2>/dev/null | xargs echo -n)
[ -z "$commit_message" ] && commit_message="unable to get the commit message"
git_old=$(sed -n 3p ./source/version.h 2>/dev/null | cut -d '"' -f 2)

if [ "$rev_new" != "$rev_old" ] || [ "$git_new" != "$git_old" ]; then
	rm -f ./source/version.h 2>/dev/null
	echo "// Don't manually edit this file" >> ./source/version.h
	echo "#define LOADER_REV \"$rev_new\"" >> ./source/version.h
	echo "#define GIT_VER \"$git_new\"" >> ./source/version.h
fi

if [ "$git_new" != "$git_old" ]; then
	if [ -z "$git_old" ]; then
		echo "Created version.h and set the commit ID to $git_new ($commit_message)" >&2
	else
		echo "Changed the commit ID from $git_old to $git_new ($commit_message)" >&2
	fi
	
	echo >&2
fi

# The HBC requires this date format for app sorting
rev_date=`date -u +%Y%m%d%H%M%S`
cat <<EOF > ./HBC/meta.xml
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<app version="1">
	<name>MarshallsLoader</name>
	<coder>blackb0x</coder>
	<version>Experimental r$rev_new</version>
	<release_date>$rev_date</release_date>
	<!-- to enable arguments change disabled_arguments to arguments -->
	<disabled_arguments>
		<arg>--ios=249</arg>
		<arg>--bootios=58</arg>
		<arg>--usbport=0</arg>
		<arg>--sdmode=1</arg>
	</disabled_arguments>
	<ahb_access/>
	<short_description>Experimental library and homebrew browser</short_description>
	<long_description>MarshallsLoader is an experimental USB Loader GX derivative. It uses apps/marshallsloader for its own settings and caches, alongside the normal GX installation. Homebrew apps appear in the library. A separately supplied local theme loads automatically. Original USB Loader GX authors retain credit. This test build is not a replacement for your normal loader.

Home:
https://github.com/wiidev/usbloadergx
Support:
https://gbatemp.net/threads/149922</long_description>
</app>
EOF
