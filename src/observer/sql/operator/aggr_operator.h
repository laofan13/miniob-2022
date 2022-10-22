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
#include "sql/parser/parse.h"
#include "rc.h"

class SelectStmt;

class AggrOperator : public Operator
{
public:
  AggrOperator(SelectStmt *select_stmt)
    : select_stmt_(select_stmt)
  {}

  ~AggrOperator(){
    // for(auto &value: aggregation_values_) {
    //   delete value.data;
    // }
  }

  RC open() override;
  RC next() override;
  RC close() override;

  Tuple * current_tuple() override {
    return nullptr;
  }
  //int tuple_cell_num() const override
  //RC tuple_cell_spec_at(int index, TupleCellSpec &spec) const override
  void init_aggregation_value();
  void aggre_value(Tuple *tuple);

  std::vector<Value> &aggregation_values() {
    return aggregation_values_;
  }

  size_t tuple_num() {
    return tuple_num_;
  }

private:
  SelectStmt *select_stmt_ = nullptr;
  std::vector<Value> aggregation_values_;
  size_t tuple_num_ = 0;
};
