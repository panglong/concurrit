

import poplib
import email

pop_conn = poplib.POP3_SSL('pop.gmail.com', 995)
pop_conn.user('recent:tayfunelmas@gmail.com')
pop_conn.pass_('10622541272')
print("Connecting")
#Get messages from server:
messages = [pop_conn.retr(i) for i in range(1, 10)]
print("Connected")
#Parse message intom an email object:
messages = [email.message_from_string("\n".join([str(b) for b in mssg[1]])) for mssg in messages]
for message in messages:
    print(message)
pop_conn.quit()
