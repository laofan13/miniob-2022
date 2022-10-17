python3 miniob_test.py \
        --test-case-dir=./test  \
        --test-case-scores=case-scores.json \
        --test-result-dir=result \
        --test-result-tmp-dir=./result_tmp \
        --test-cases=primary-drop-table \
        --db-base-dir=/home/ubuntu/DS/oceanbase/miniob-2022/build \
        --db-config=/home/ubuntu/DS/oceanbase/miniob-2022/etc/observer.ini \
        --code-type = none \
        --server-started \
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

2. COMPARE DATE DATA
SELECT * FROM date_table WHERE u_date>'2020-1-20';
1 | 2020-01-21
+11 | 2042-02-02
2 | 2020-10-21
9 | 2038-01-19
-11 | 2042-02-02
ID | U_DATE
SELECT * FROM date_table WHERE u_date<'2019-12-31';
+10 | 1950-02-02