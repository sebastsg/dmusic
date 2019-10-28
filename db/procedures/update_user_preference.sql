create or replace procedure update_user_preference (
    in in_user_name  varchar(32),
    in in_type       int,
    in in_preference int
)
language plpgsql
as $$
begin
    insert into "user_preference" ("user_name", "type", "preference")
         values (in_user_name, in_type, in_preference)
    on conflict 
        on constraint pk_user_preference
            do update
                  set "preference" = in_preference;
end;
$$;
