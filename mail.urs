(** Ur/Web e-mail sending library *)

(* To assemble a message, produce a value in this type standing for header values. *)
type headers

val empty : headers

(* Each of the following may be used at most once in constructing a [headers]. *)
val from : string -> headers -> headers
val subject : string -> headers -> headers
val replyto : string -> headers -> headers

(* The following must be called with single valid e-mail address arguments, and
 * all such addresses passed are combined into single header values. *)
val to : string -> headers -> headers
val cc : string -> headers -> headers
val bcc : string -> headers -> headers

(* Send out a message by connecting to an SMTP server on localhost:25. *)
val send : headers
           -> string       (* Plain text message body *)
           -> option xbody (* Optional HTML message body *)
           -> transaction unit
