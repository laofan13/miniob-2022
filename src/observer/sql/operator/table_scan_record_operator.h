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
// Created by WangYunlai on 2021/6/7.
//

#pragma once

#include "sql/operator/operator.h"
#include "storage/record/record_manager.h"
#include "rc.h"

class Table;

class TableScanRecordOperator : public Operator
{
public:
  TableScanRecordOperator(Table *table)
    : table_(table)
  {
    current_index_ = 0;
  }

  virtual ~TableScanRecordOperator() {
    for (auto tuple: table_tuples) {
        delete tuple;
    }
    table_tuples.clear();

    for (auto record: table_records) {
        delete record;
    }
    table_records.clear();
  }

  RC open() override;
  RC next() override;
  RC close() override;

  Tuple * current_tuple() override;

  // int tuple_cell_num() const override
  // {
  //   return tuple_.cell_num();
  // }

  // RC tuple_cell_spec_at(int index, TupleCellSpec &spec) const override;
private:
  Table *table_ = nullptr;
  RecordFileScanner record_scanner_;
  std::vector<Record*> table_records;
  std::vector<Tuple*> table_tuples;
  int current_index_;
  Tuple* tuple_;
};
