#!/bin/sh
perl ./cstyle.pl common/src/* st/src/* stsw/src/* common/h/* st/h/* stsw/h/* | grep -v -e 'extra space between function name and left paren' -e 'missing blank after open comment' -e 'improper first line of block comment'
