create or replace procedure delete_session_track (
    in in_user_name  varchar(32),
    in in_num        int
)
language plpgsql
as $$
begin
    delete from "session_track"
          where "user_name" = in_user_name
            and "num" = in_num;

    update "session_track"
       set "num" = "num" - 1
     where "user_name" = in_user_name
       and "num" > in_num;
end;
$$;
