
#include <algorithm>
#include<vector>
#include "common/log/log.h"
#include "sql/operator/sort_operator.h"
#include "storage/common/field.h"

RC SortOperator::open()
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

  while (RC::SUCCESS == (rc = child->next())) {
    Tuple *tuple = child->current_tuple();
    if (nullptr == tuple) {
      LOG_WARN("failed to get current record: %s", strrc(rc));
      return rc;
    }
    table_tuples_.push_back(tuple);
  }

  auto sort_fields = sort_fields_;
  std::sort(table_tuples_.begin(),table_tuples_.end(), [sort_fields](Tuple *t1, Tuple *t2) {
    int ret = 0;
    TupleCell cell1;
    TupleCell cell2;
    int size = sort_fields->size();
    SortField &sort_field = sort_fields->at(0);
    for(int i = 0; i < sort_fields->size();i++) {
      SortField &sort_field = sort_fields->at(i);

      t1->find_cell(sort_field, cell1);
      t2->find_cell(sort_field, cell2);

      if(sort_field.order_type() == ORDER_ASC) {
        if(cell1.attr_type() == NULLS && cell2.attr_type() == NULLS) {// 有空值
          ret = 0;
        }else if(cell1.attr_type() == NULLS && cell2.attr_type() != NULLS){
          ret = -1;
        }else if(cell1.attr_type() != NULLS && cell2.attr_type() == NULLS){
          ret = 1;
        }else{
          ret = cell2.compare(cell1);
        }
      }else{
        if(cell1.attr_type() == NULLS && cell2.attr_type() == NULLS) {// 有空值
          ret = 0;
        }else if(cell1.attr_type() == NULLS && cell2.attr_type() != NULLS){
          ret = 1;
        }else if(cell1.attr_type() != NULLS && cell2.attr_type() == NULLS){
          ret = -1;
        }else{
          ret = cell1.compare(cell2);
        }
      }

      if(ret > 0) {
        return true;
      }else if(ret < 0) {
        return false;
      }
    }
    return false;
  });

  return RC::SUCCESS;
}

RC SortOperator::next()
{
  if(current_index_ < table_tuples_.size()) { 
    tuple_ = table_tuples_[current_index_++];
    return RC::SUCCESS;
  }
  return RC::RECORD_EOF;
}

RC SortOperator::close()
{
  children_[0]->close();
  return RC::SUCCESS;
}

Tuple * SortOperator::current_tuple()
{
  return tuple_;
}