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
// Created by lifengfan on 2022/10/12.
//

#include "common/log/log.h"
#include "sql/operator/update_operator.h"
#include "storage/record/record.h"
#include "storage/common/table.h"
#include "sql/stmt/delete_stmt.h"

RC UpdateOperator::open()
{
  if (children_.size() != 1) {
    LOG_WARN("delete operator must has 1 child");
    return RC::INTERNAL;
  }
  RC rc = RC::SUCCESS;

  rc = children_[0]->open();
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to open child operator: %s", strrc(rc));
    return rc;
  }

  Table *table = update_stmt_->table();
  const char *attribute_name = update_stmt_->field().field_name();
  const Value *value = update_stmt_->value();
  Operator *child = children_[0];
  while (RC::SUCCESS == (rc = child->next())) {
    Tuple *tuple = child->current_tuple();
    if (nullptr == tuple) {
        LOG_WARN("failed to get current record: %s", strrc(rc));
        return rc;
    }
    RowTuple *row_tuple = static_cast<RowTuple *>(tuple);
    Record &record = row_tuple->record();
    
    rc = table->update_record(nullptr, attribute_name, value, &record);
    if (rc != RC::SUCCESS) {
        LOG_WARN("failed to delete record: %s", strrc(rc));
        return rc;
    }
  }
  return RC::SUCCESS;
}

RC UpdateOperator::next()
{
  return RC::RECORD_EOF;
}

RC UpdateOperator::close()
{
  children_[0]->close();
  return RC::SUCCESS;
}