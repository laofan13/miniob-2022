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

#include "sql/operator/table_scan_record_operator.h"
#include "storage/common/table.h"
#include "rc.h"

RC TableScanRecordOperator::open()
{
  RC rc = table_->get_record_scanner(record_scanner_);
  if (rc != RC::SUCCESS) {
    return rc;
    // tuple_.set_schema(table_, table_->table_meta().field_metas());
  }

  Record current_record;
  while (record_scanner_.has_next()) {
    rc = record_scanner_.next(current_record);
    if (rc != RC::SUCCESS) {
      return rc;
    }

    Record *record = new Record(current_record);
    RowTuple *row_tuple = new RowTuple(table_, table_->table_meta().field_metas());
    row_tuple->set_record(record);
    

    table_records.push_back(record);
    table_tuples.push_back(row_tuple); 
  }

  return rc;
}

RC TableScanRecordOperator::next()
{
  if(current_index_ < table_tuples.size()) {
    tuple_ = table_tuples[current_index_++];
    return RC::SUCCESS;
  }
  return RC::RECORD_EOF;
}

RC TableScanRecordOperator::close()
{
  return record_scanner_.close_scan();
}

Tuple * TableScanRecordOperator::current_tuple()
{
  return tuple_;
}