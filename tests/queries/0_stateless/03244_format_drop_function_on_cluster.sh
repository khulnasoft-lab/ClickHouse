#!/usr/bin/env bash

CURDIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
# shellcheck source=../shell_config.sh
. "$CURDIR"/../shell_config.sh

echo "
CREATE FUNCTION linear_equation AS (x, k, b) -> ((k * x) + b);

DROP FUNCTION linear_equation ON CLUSTER test;
" | ${CLICKHOUSE_FORMAT} --multiquery
