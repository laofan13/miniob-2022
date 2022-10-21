/* Copyright (c) 2021 Xie Meiyi(xiemeiyi@hust.edu.cn) and OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

//
// Created by WangYunlai on 2021/6/9.
//

#include "sql/operator/insert_operator.h"
#include "sql/stmt/insert_stmt.h"
#include "storage/common/table.h"
#include "rc.h"

RC InsertOperator::open()
{
  Table *table = insert_stmt_->table();
  const InsertRecord *records = insert_stmt_->records();
  int record_amount = insert_stmt_->record_amount();
 
  RC rc = RC::SUCCESS;
  for(int i = 0; i < record_amount; i++) {
      // check the fields number
    const Value *values = records[i].values;
    const int value_num = records[i].value_num;
    rc = table->insert_record(nullptr, value_num, values); // TODO trx

    if(rc != RC::SUCCESS) {
      LOG_ERROR("Failed to inser a record. rc=%d:%s", rc, strrc(rc));
      return rc;
    }
  }
}

RC InsertOperator::next()
{
  return RC::RECORD_EOF;
}

RC InsertOperator::close()
{
  return RC::SUCCESS;
}
