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
// Created by Wangyunlai on 2022/07/05.
//

#include "sql/expr/tuple.h"


RC FieldExpr::get_value(const Tuple &tuple, TupleCell &cell) const
{
  return tuple.find_cell(field_, cell);
}

RC ValueExpr::get_value(const Tuple &tuple, TupleCell & cell) const
{
  cell = tuple_cell_;
  return RC::SUCCESS;
}

RC FuncFieldExpr::get_value(const Tuple &tuple, TupleCell & cell) const
{
  RC rc = tuple.find_cell(field(), cell);
  switch (func_type_)
  {
  case MAX_FUNC: {

  }break;
  case MIN_FUNC: {

  }break;
  case COUNT_FUNC: {

  }break;
  case AVG_FUNC: {

  }break;
  case SUM_FUNC: {

  }break;
  case LENGTH_FUNC: {

  }break;
  case ROUND_FUNC: {

  }break;
  case DATE_FORMAT_FUNC: {

  }break;
  default:
    break;
  }

  return RC::SUCCESS;
}

RC FuncValueExpr::get_value(const Tuple &tuple, TupleCell & cell) const
{
  get_tuple_cell(cell);

  switch (func_type_)
  {
  case MAX_FUNC: {

  }break;
  case MIN_FUNC: {

  }break;
  case COUNT_FUNC: {

  }break;
  case AVG_FUNC: {

  }break;
  case SUM_FUNC: {

  }break;
  case LENGTH_FUNC: {

  }break;
  case ROUND_FUNC: {

  }break;
  case DATE_FORMAT_FUNC: {

  }break;
  default:
    break;
  }

  return RC::SUCCESS;
}