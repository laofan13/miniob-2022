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

#include "sql/operator/aggr_operator.h"
#include "sql/stmt/select_stmt.h"
#include "storage/common/table.h"
#include "rc.h"
#include "float.h"
#include "util/comparator.h"

void AggrOperator::init_aggregation_value() {
  // init aggregation value
  for(auto aggr_field: select_stmt_->aggr_fields()) {
    AggrType aggr_type = aggr_field.aggr_type();
    AttrType attr_type = aggr_field.attr_type();

    Value value;
    value.type = attr_type;
    value.data = new char[4];

    switch (aggr_type)
    {
    case COUNT_FUNC: {
      *(int *)(value.data) = 0;
    }break;
    case MAX_FUNC:{
      if(attr_type == INTS || attr_type == DATES) {
        *(int *)(value.data) = INT32_MIN;
      }else if(attr_type == FLOATS) {
        *(float *)(value.data) = FLT_MIN;
      }else if(attr_type == CHARS) {
        char *str = (char *)value.data;
        for(int i = 0;i < 3;i++) {
          str[i] = 0;
        }
        str[3] = '\0';
      }
    }break;
    case MIN_FUNC:{
      if(attr_type == INTS || attr_type == DATES) {
        *(int *)(value.data) = INT32_MAX;
      }else if(attr_type == FLOATS) {
        *(float *)(value.data) = FLT_MAX;
      }else if(attr_type == CHARS) {
        char *str = (char *)value.data;
        for(int i = 0;i < 3;i++) {
          str[i] = 127;
        }
        str[3] = '\0';
      }
    }break;
    case AVG_FUNC:{
      if(attr_type == INTS || attr_type == DATES) {
        *(int *)(value.data) = 0;
      }else if(attr_type == FLOATS) {
        *(float *)(value.data) = 0;
      }
    }break;
    case SUM_FUNC:{
      if(attr_type == INTS || attr_type == DATES) {
        *(int *)(value.data) = 0;
      }else if(attr_type == FLOATS) {
        *(float *)(value.data) = 0;
      }
    }break;
    default:
      break;
    }

    aggregation_values_.push_back(value);
  }
}

void AggrOperator::aggre_value(Tuple *tuple) {
  auto aggr_fields = select_stmt_->aggr_fields();
  RowTuple *row_tuple = static_cast<RowTuple *>(tuple);
  TupleCell cell;

  for(size_t i = 0; i < aggr_fields.size();i++) {
    AggrField &aggr_field = aggr_fields[i];
    AggrType aggr_type = aggr_field.aggr_type();
    AttrType attr_type = aggr_field.attr_type();

    Value &value = aggregation_values_[i];

    if(aggr_type != COUNT_FUNC) {
      row_tuple->find_cell(aggr_field,cell);
    } 
    
    switch (aggr_type)
    {
    case COUNT_FUNC: {
      *(int *)(value.data) += 1;
    }break;
    case MAX_FUNC:{
      if(attr_type == INTS || attr_type == DATES) {
        int cell_value = *(int *)cell.data();
        if(cell_value > *(int *)(value.data)) {
          *(int *)(value.data) = cell_value;
        }
      }else if(attr_type == FLOATS) {
        float cell_value = *(float *)cell.data();
        if(cell_value > *(float *)(value.data)) {
          *(float *)(value.data) = cell_value;
        }
      }else if(attr_type == CHARS) {
        char *str = (char *)(value.data);
        char *cell_value = (char *)cell.data();
        if(compare_string(cell_value, 3, str, 3) > 0) {
          for(int i = 0;i < 3;i++) {
            str[i] = cell_value[i];
          }
          str[3] = '\0';
        }
      }
    }break;
    case MIN_FUNC:{
      if(attr_type == INTS || attr_type == DATES) {
        int cell_value = *(int *)cell.data();
        if(cell_value < *(int *)(value.data)) {
          *(int *)(value.data) = cell_value;
        }
      }else if(attr_type == FLOATS) {
        float cell_value = *(float *)cell.data();
        if(cell_value < *(float *)(value.data)) {
          *(float *)(value.data) = cell_value;
        }
      }else if(attr_type == CHARS) {
        char *str = (char *)(value.data);
        char *cell_value = (char *)cell.data();
        if(compare_string(cell_value, 3, str, 3) < 0) {
          for(int i = 0;i < 3;i++) {
            str[i] = cell_value[i];
          }
          str[3] = '\0';
        }
      }
    }break;
    case AVG_FUNC:{
      if(attr_type == INTS || attr_type == DATES) {
        int cell_value = *(int *)cell.data();
        *(int *)(value.data) += cell_value;
      }else if(attr_type == FLOATS) {
        float cell_value = *(float *)cell.data();
        *(float *)(value.data) += cell_value;
      }
    }break;
    case SUM_FUNC:{
      if(attr_type == INTS || attr_type == DATES) {
        int cell_value = *(int *)cell.data();
        *(int *)(value.data) += cell_value;
      }else if(attr_type == FLOATS) {
        float cell_value = *(float *)cell.data();
        *(float *)(value.data) += cell_value;
      }
    }break;
    default:
      break;
    }
  }
}

RC AggrOperator::open()
{
  if (children_.size() != 1) {
    LOG_WARN("delete operator must has 1 child");
    return RC::INTERNAL;
  }

  Operator *child = children_[0];
  RC rc = child->open();
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to open child operator: %s", strrc(rc));
    return rc;
  }
  // init aggregation
  init_aggregation_value();
  while (RC::SUCCESS == (rc = child->next())) {
    Tuple *tuple = child->current_tuple();
    if (nullptr == tuple) {
      LOG_WARN("failed to get current record: %s", strrc(rc));
      return rc;
    }
    tuple_num_++;
    aggre_value(tuple);
  }

  auto aggr_fields = select_stmt_->aggr_fields();
  for(size_t i = 0; i < aggr_fields.size();i++) {
    AggrField &aggr_field = aggr_fields[i];
    AggrType aggr_type = aggr_field.aggr_type();
    AttrType attr_type = aggr_field.attr_type();

    Value &value = aggregation_values_[i];

    if(aggr_type == AVG_FUNC) {
      if(attr_type == INTS || attr_type == DATES) {
        *(int *)(value.data) /= tuple_num_;
      }else if(attr_type == FLOATS) {
        *(float *)(value.data) /=(float)tuple_num_;
      }
    } 
  }

  return RC::SUCCESS;
}

RC AggrOperator::next()
{
  return RC::RECORD_EOF;
}

RC AggrOperator::close()
{
  children_[0]->close();
  return RC::SUCCESS;
}
