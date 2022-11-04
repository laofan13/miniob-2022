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
// Created by lifengfan on 2022/10/11.
//


#pragma once

#include "sql/operator/operator.h"
#include "sql/operator/table_scan_operator.h"

class FilterStmt;

class DescartesOperator : public Operator
{
public:
  DescartesOperator(std::vector<TableScanOperator*> scan_opers)
  :scan_opers_(scan_opers)
  {
    total_num_ = 1;
    current_index_ = 0;
  }

  virtual ~DescartesOperator(){
    for (auto scan_oper : scan_opers_) {
      delete scan_oper;
    }
    scan_opers_.clear();

    for (auto &vec_tuples: table_tuples) {
      for (auto tuple: vec_tuples) {
        delete tuple;
      }
      vec_tuples.clear();
    }
    table_tuples.clear();

    for (auto &vec_records: table_records) {
      for (auto record: vec_records) {
        delete record;
      }
      vec_records.clear();
    }
    table_records.clear();

    for(Tuple* tuple: composite_table_tuples_) {
        delete tuple;
    }
    composite_table_tuples_.clear();
  }

  RC open() override;
  RC next() override;
  RC close() override;

  Tuple * current_tuple() override;
  //int tuple_cell_num() const override;
  //RC tuple_cell_spec_at(int index, TupleCellSpec &spec) const override;
private:
  
private:
  std::vector<TableScanOperator*> scan_opers_;
  std::vector<std::vector<Record*> > table_records;
  std::vector<std::vector<Tuple*> > table_tuples;
  std::vector<std::pair<int, int>> index_mul_;
  int total_num_;
  int current_index_;

  std::vector<CompositeTuple*> composite_table_tuples_;
  Tuple *tuple_;
};