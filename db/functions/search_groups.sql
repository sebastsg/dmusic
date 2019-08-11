create or replace function search_groups (
    in in_name  varchar(160),
    in in_limit int
)
returns table (
    "id"   int,
    "name" varchar
)
language plpgsql
as $$
begin
    return query
    select "group"."id",
           "group"."name"
      from "group"
     where lower("group"."name") like lower(in_name)
     limit in_limit;
end;
$$;
