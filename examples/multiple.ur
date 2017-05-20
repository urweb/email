
fun solicitText (user : string) : transaction string
  = key <- rand;
    pass <- return (show key);
    return ("Dear ...,\n\n" ^
            "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nunc pellentesque porta" ^
            "congue. Proin quis est est. Pellentesque facilisis at arcu a bibendum. Curabitur" ^
            "non sapien quis felis tincidunt pulvinar. Curabitur id orci augue. Mauris auctor" ^
            "augue non nisi ornare elementum. Aenean scelerisque urna at nulla interdum cursus." ^
            "Nunc finibus augue nunc, eu sodales augue lobortis in. Nullam sed molestie eros." ^
            "Aenean ullamcorper tellus turpis, vel efficitur eros pellentesque ut. Morbi vitae" ^
            "porta ex, ut aliquam urna. Nunc fringilla velit diam, eget convallis felis iaculis" ^
            "in.\n" ^
            "Donec varius, augue vitae maximus laoreet, est erat laoreet lectus, id convallis" ^
            "velit lectus a leo. Etiam id luctus quam. Nunc et imperdiet libero. Vivamus sit amet" ^
            "urna sit amet ipsum efficitur pretium quis in neque. Suspendisse quam magna," ^
            "pharetra auctor vestibulum sit amet, ultrices in dolor. Donec in dapibus nulla, sit" ^
            "amet ornare augue. Etiam commodo lacus tellus, sed gravida ligula varius" ^
            "eu. Maecenas ante dolor, sagittis ut commodo ut, condimentum ac augue. Aenean" ^
            "facilisis pellentesque tellus, nec molestie enim feugiat non. Curabitur ipsum augue," ^
            "efficitur eget scelerisque id, rhoncus eget enim. Cras bibendum blandit leo, et" ^
            "sagittis risus commodo sed. Praesent consectetur nulla ut porttitor blandit. Nunc" ^
            "pretium dictum libero, vel lobortis mi tempus eget. Aenean hendrerit ultricies mi," ^
            "in semper velit accumsan a.\n" ^
            "Im futrem usume " ^ show pass ^ "\n\n" ^
            "Sincerely,\n\n" ^
            "Your System.\n"
            )

val sendOneMail (from : string) (to : string) (subject : string) (text : string) : transaction unit
  = Mail.send (Mail.subject subject (Mail.to to (Mail.from from Mail.empty))) text None
              
val sendMails : transaction unit
  = users <- return ("urweb.test1@mailinator.com" :: "urweb.test2@mailinator.com" :: "urweb.test3@mailinator.com" :: []);
    List.app
        (fn user =>
            text <- solicitText user;
            sendOneMail "MarkoSchuetz@gmail.com" user "soliciting your data" text
        )
        users

fun main () : transaction page
  = sendMails;
    return <xml>Sent</xml>
