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

class JoinOperator : public Operator
{
public:
  JoinOperator(std::vector<Operator*> scan_opers,FilterStmt *filter_stmt)
  :scan_opers_(scan_opers), filter_stmt_(filter_stmt)
  {
    total_num_ = 1;
    current_index_ = 0;
  }

  virtual ~JoinOperator(){
    for (auto scan_oper : scan_opers_) {
      delete scan_oper;
    }
    scan_opers_.clear();

    for (auto &vec_tuples: table_tuples_) {
      for (auto tuple: vec_tuples) {
        delete tuple;
      }
      vec_tuples.clear();
    }
    table_tuples_.clear();

    for (auto &vec_records: table_records_) {
      for (auto record: vec_records) {
        delete record;
      }
      vec_records.clear();
    }
    table_records_.clear();

  }

  RC open() override;
  RC next() override;
  RC close() override;

  Tuple * current_tuple() override;
  //int tuple_cell_num() const override;
  //RC tuple_cell_spec_at(int index, TupleCellSpec &spec) const override;
private:
  bool do_predicate(Tuple *tuple);
private:
  std::vector<Operator*> scan_opers_;
  std::vector<std::vector<Record*>> table_records_;
  std::vector<std::vector<Tuple*>> table_tuples_;
  std::vector<std::pair<int, int>> index_mul_;
  long long total_num_;
  long long current_index_;
  CompositeTuple tuple_;
  FilterStmt *filter_stmt_ = nullptr;
};