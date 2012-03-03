fun sendPlain r =
    Mail.send (Mail.from r.From (Mail.to r.To (Mail.subject r.Subject Mail.empty)))
              r.Body None;
    return <xml>Sent</xml>

fun sendHtml r =
    Mail.send (Mail.from r.From (Mail.to r.To (Mail.subject r.Subject Mail.empty)))
              r.Body (Some <xml><a href={url (main ())}>Spread the love!</a></xml>);
    return <xml>Sent</xml>

and main () = return <xml><body>
  <h2>Plain</h2>

  <form>
    From: <textbox{#From}/><br/>
    To: <textbox{#To}/><br/>
    Subject: <textbox{#Subject}/><br/>
    Body: <textarea{#Body}/><br/>
    <submit action={sendPlain}/>
  </form>

  <h2>HTML</h2>

  <form>
    From: <textbox{#From}/><br/>
    To: <textbox{#To}/><br/>
    Subject: <textbox{#Subject}/><br/>
    Body: <textarea{#Body}/><br/>
    <submit action={sendHtml}/>
  </form>
</body></xml>
