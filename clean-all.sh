#!/bin/bash
# $Author$
# $Revision$
# $Date$

find . -depth \( -name Documentation -or -name '*Debug' -or -name '*Release' -or -name '*Profile' -or -name '*.suo' -or -name '*.ncb' -or -name '*cache.dat' \) -exec rm -rf '{}' \;