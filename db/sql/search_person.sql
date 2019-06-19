create or replace function search_person (
    in in_name  varchar(128),
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
    select "person"."id",
           concat("first_name", ' ', "last_name")
      from "person"
     where concat("first_name", ' ', "last_name") like in_name
     limit in_limit;
end;
$$;
