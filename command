# basic
python3 miniob_test.py \
        --test-case-dir=./test  \
        --test-case-scores=case-scores.json \
        --test-result-dir=result \
        --test-result-tmp-dir=./result_tmp \
        --test-cases=basic \
        --db-base-dir=/home/ubuntu/DS/oceanbase/miniob-2022/build \
        --db-config=/home/ubuntu/DS/oceanbase/miniob-2022/etc/observer.ini \
        --code-type = none \
        --debug \
        --log=stdout

# primary-select-meta
python3 miniob_test.py \
        --test-case-dir=./test  \
        --test-case-scores=case-scores.json \
        --test-result-dir=result \
        --test-result-tmp-dir=./result_tmp \
        --test-cases=primary-select-meta \
        --db-base-dir=/home/ubuntu/DS/oceanbase/miniob-2022/build \
        --db-config=/home/ubuntu/DS/oceanbase/miniob-2022/etc/observer.ini \
        --code-type = none \
        --debug \
        --log=stdout

# primary-drop-table
python3 miniob_test.py \
        --test-case-dir=./test  \
        --test-case-scores=case-scores.json \
        --test-result-dir=result \
        --test-result-tmp-dir=./result_tmp \
        --test-cases=primary-drop-table \
        --db-base-dir=/home/ubuntu/DS/oceanbase/miniob-2022/build \
        --db-config=/home/ubuntu/DS/oceanbase/miniob-2022/etc/observer.ini \
        --code-type = none \
        --debug \
        --log=stdout

# primary-update
python3 miniob_test.py \
        --test-case-dir=./test  \
        --test-case-scores=case-scores.json \
        --test-result-dir=result \
        --test-result-tmp-dir=./result_tmp \
        --test-cases=primary-update \
        --db-base-dir=/home/ubuntu/DS/oceanbase/miniob-2022/build \
        --db-config=/home/ubuntu/DS/oceanbase/miniob-2022/etc/observer.ini \
        --code-type = none \
        --debug \
        --log=stdout

# primary-date
python3 miniob_test.py \
        --test-case-dir=./test  \
        --test-case-scores=case-scores.json \
        --test-result-dir=result \
        --test-result-tmp-dir=./result_tmp \
        --test-cases=primary-date\
        --db-base-dir=/home/ubuntu/DS/oceanbase/miniob-2022/build \
        --db-config=/home/ubuntu/DS/oceanbase/miniob-2022/etc/observer.ini \
        --code-type = none \
        --debug \
        --log=stdout

# primary-select-tables
python3 miniob_test.py \
        --test-case-dir=./test  \
        --test-case-scores=case-scores.json \
        --test-result-dir=result \
        --test-result-tmp-dir=./result_tmp \
        --test-cases=primary-select-tables \
        --db-base-dir=/home/ubuntu/DS/oceanbase/miniob-2022/build \
        --db-config=/home/ubuntu/DS/oceanbase/miniob-2022/etc/observer.ini \
        --code-type = none \
        --debug \
        --log=stdout

