create or replace procedure get_all_group_thumbs ()
language plpgsql
as $$
begin
    select "id",
           "name"
      from "group";
end;
$$;
