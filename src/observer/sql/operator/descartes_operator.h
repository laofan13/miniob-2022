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
#include "rc.h"

class FilterStmt;

class DescartesOperator : public Operator
{
public:
  DescartesOperator(FilterStmt *filter_stmt)
    : filter_stmt_(filter_stmt)
  {}

  virtual ~DescartesOperator() = default;

  RC open() override;
  RC next() override;
  RC close() override;

  Tuple * current_tuple() override;
  //int tuple_cell_num() const override;
  //RC tuple_cell_spec_at(int index, TupleCellSpec &spec) const override;
private:
  void descartes(int i,std::vector<Tuple*> cur_result);
  bool do_predicate(CompositeTuple &tuple) ;
  
private:
  FilterStmt *filter_stmt_ = nullptr;
  std::vector<std::vector<Record> > dim_record_;
  std::vector<std::vector<RowTuple> > dim_tuples_;
  std::vector<std::vector<Tuple*> > descartes_result_;
  int pos_ = 0;
  CompositeTuple tuple_;
};