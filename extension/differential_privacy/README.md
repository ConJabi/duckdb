This directory contains preliminary work to include OpenDP into DuckDP

---
### Compiling OpenDP library
run the compile_opendp.sh script

### Create Table and Insert Data

First, we create a table named `a` with a single column `i` of type `double`, and then insert a value.

```sql
create table a (i double);
insert into a values(5.0);
```

### Create Table and Insert Data

First, we create a table named `a` with a single column `i` of type `double`, and then insert a value.

```sql
create table a (i double);
insert into a values(5.0);
```

### Make table private
pragma make_table_private(TABLE_NAME);<br> limits the operations allowed on the table

 pragma add_bounds(TABLE_NAME,COLUMN_NAME, LOWER_BOUND, UPPER_BOUND); <br>
 adds boundary information to the column, these bounds are enforced and used when computing private aggregations

pragma add_replacement(TABLE_NAME,COLUMN_NAME, REPLACEMENT_VALUE); <br>
Value used when replacing NULL values




```sql
pragma make_table_private("a");
select sum(i);
-- Permission Error: Table is private
pragma add_bounds("a","i", 2.0, 4.0);
-- success
pragma add_replacement(4.0);
-- success
select sum(i) from a;
--  3.925

```

