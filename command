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


SELECT Select_tables_2.age FROM Select_tables_1,Select_tables_2 WHERE Select_tables_1.age<18 AND Select_tables_2.u_name='c' AND Select_tables_1.id=Select_tables_2.id;
21
SELECT_TABLES_2.AGE
-SELECT Select_tables_1.id,Select_tables_4.u_name,Select_tables_5.res FROM Select_tables_1,Select_tables_4,Select_tables_5 where Select_tables_4.age>2 AND Select_tables_5.u_name!='f';
-1 | B | 10
-1 | B | 12
-1 | D | 10
-1 | D | 12
-2 | B | 10
-2 | B | 12