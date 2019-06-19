create or replace function search_country (
    in in_name  varchar(128),
    in in_limit int
)
returns table (
    "code" varchar,
    "name" varchar
)
language plpgsql
as $$
begin
    return query
    select "country"."code",
           "country"."name"
      from "country"
     where "country"."name" like in_name
     limit in_limit;
end;
$$;
