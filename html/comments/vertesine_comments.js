var request = new XMLHttpRequest();
var comment_id = String(window.location.href);
var reply_comment_id = 0;
var comment_body;
var final_comment = {
  parent_id:0,
  page_name:0,
  comment_text:0
};
var comment_div = document.getElementById("comments");
comment_id = encodeURI(comment_id.replace(/^https?:\/\//,''));

var html_string =
"<link rel=\"stylesheet\" href=\"/comments/vertesine_comments_style.css\">" +
"<div class=\"vertesine_comment_top_container\">" +
"<br><font size=\"6pt\" color=\"black\">Post a comment(in progress)</font><br>" +
"<font id=\"vertesine_comment_warning\" size=\"3pt\" color=\"red\"></font><br>" +
"<textarea id=\"vertesine_comment_body_area\" style=\"background-color: #2D2D2D; color: #EFEFEF; width: 100%; height: 160px; box-sizing: border-box;\"></textarea>" +
"<button onclick=\"post_comment();\">SUBMIT</button>" +
"</div>" +
"<hr></hr>" +
"<div class=\"vertesine_comment_comment_container\">" +
"<div class=\"vertesine_comment_title_av_container\">" +
"<img src=\"/default_usericons/usericon.png\" alt=\"usericon\" class=\"vertesine_comment_profile_page_av\"" +
"<br>USERNAME<br><i>TEST</i>" +
"</div>" +
"<div class=\"vertesine_comment_body_text\">" +
"comment. Hello" +
"<br><br><button onclick=\"reply_comment();\">reply</button>"
"</div" +
"</div>";





var post_comment = function()
{
  comment_body = document.getElementById("vertesine_comment_body_area").value;
  console.log("aaaa thing " + comment_body);


  if(comment_body.length > 600)
  {
    document.getElementById("vertesine_comment_warning").innerHTML = "TOO MANY CHARACTERS. TRY AND KEEP IT UNDER 600";
  }
  else if(comment_body.length > 0)
  {
    final_comment.comment_text = comment_body;
    final_comment.page_name = String(window.location.href).replace(/^https?:\/\//,'');
    final_comment.parent_id = reply_comment_id;

    document.getElementById("vertesine_comment_warning").innerHTML = "";
    request.open("POST", "/comments/post_comment/", true);
    request.setRequestHeader('Content-type', 'application/x-www-form-urlencoded');


    request.onreadystatechange = function()
    {
      if(this.readyState == 4)
      {
        console.log("posted comment");

        if(this.responseText == "unauthorized")
        {
          document.getElementById("vertesine_comment_warning").innerHTML = "PLEASE LOG IN";
        }
      }
    };

    request.send("c=" + encodeURIComponent(JSON.stringify(final_comment)));
  }
  else
  {
    document.getElementById("vertesine_comment_warning").innerHTML = "PLEASE WRITE SOMETHING";
  }


}

var update_comments = function(json_data)
{
  var parent_container;

  try
  {
    parent_container = JSON.parse(json_data);
  }
  catch(e)
  {
    console.log("big error " + json_data);

    return 1;
  }
  console.log("parsing");




  if(parent_container.status == "NO_COMMENTS")
  {
    console.log("no comments " + json_data);
    //TODO display a "no comments message"
  }
  else
  {
    console.log("comments " + json_data);
  }




}

var load_comments = function(page)
{
  request.open("GET", "/comments/get_comment" + "?parent=" + comment_id, true);
  console.log("loading comments on page number " + page + " at " + comment_id);


  request.onreadystatechange = function()
  {
    if(this.readyState == 4)
    {
      console.log("updating comments");
      update_comments(this.responseText);
    }
  };

  request.send();

}

var setup_comment_div = function()
{
  comment_div.setAttribute("class", "vertesine_comment_body");
  comment_div.innerHTML = html_string.trim();
}

setup_comment_div();

load_comments(0);
