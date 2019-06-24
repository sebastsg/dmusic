create or replace procedure get_group_name (
    in in_group_id int
)
language plpgsql
as $$
begin
    select "name"
      from "group"
     where "id" = in_group_id;
end;
$$;
