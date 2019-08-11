create or replace function get_group_albums (
    in in_group_id int
)
returns table (
    "album_id"          int,
    "name"              varchar,
    "type"              varchar,
    "cover"             int,
    "album_release_id"  int,
    "release_type"      varchar,
    "catalog"           varchar,
    "released_at"       int
)
language plpgsql
as $$
begin
       return query
       select "album"."id",
              "album"."name",
              "album"."album_type_code",
              "album_attachment"."num",
              "album_release"."id",
              "album_release"."album_release_type_code",
              "album_release"."catalog",
              cast(extract(epoch from "album_release"."released_at") as int)
         from "group"
         join "album_release_group"
           on "album_release_group"."group_id" = "group"."id"
         join "album_release"
           on "album_release"."id" = "album_release_group"."album_release_id"
         join "album"
           on "album"."id" = "album_release"."album_id"
    left join "album_attachment"
           on "album_attachment"."album_release_id" = "album_release"."id"
          and "album_attachment"."type" = 'cover'
        where "group"."id" = in_group_id
     order by "album_release"."released_at" desc;
end;
$$;
