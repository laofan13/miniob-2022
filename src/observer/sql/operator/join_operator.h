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
// Created by WangYunlai on 2021/6/10.
//

#pragma once

#include "sql/parser/parse.h"
#include "sql/operator/operator.h"
#include "sql/stmt/filter_stmt.h"
#include "rc.h"

// TODO fixme
class JoinOperator : public Operator
{
public:
  JoinOperator(Operator *left, Operator *right,FilterUnit *filter_unit):
    left_(left),right_(right),filter_unit_(filter_unit)
  {
    current_index_ = 0;
    total_num_ = 0;
  }

  virtual ~JoinOperator() {
    delete left_;
    delete right_;

    for (auto record: left_record_) {
      delete record;
    }
    left_record_.clear();
    for (auto record: right_record_) {
      delete record;
    }
    right_record_.clear();

    for (auto tuple: left_tuples_) {
      delete tuple;
    }
    left_tuples_.clear();

    for (auto tuple: right_tuples_) {
      delete tuple;
    }
    right_tuples_.clear();
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
  Operator *left_ = nullptr;
  Operator *right_ = nullptr;
  bool round_done_ = true;

  std::vector<Record *> left_record_;
  std::vector<Record *> right_record_;
  std::vector<Tuple *> left_tuples_;
  std::vector<Tuple *> right_tuples_;
  FilterUnit *filter_unit_ = nullptr;

  int current_index_;
  int total_num_;
  CompositeTuple tuple_;
};
