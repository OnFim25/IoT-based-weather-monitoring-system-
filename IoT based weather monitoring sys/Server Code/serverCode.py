from flask import Flask, request, jsonify, render_template
import mongoengine as me
from datetime import datetime



app = Flask(__name__)

dbs = me.connect(host = "mongodb+srv://neerajpcvr581:neeraj1981998@weathermonitoring.r9zztni.mongodb.net/?retryWrites=true&w=majority&appName=weatherMonitoring")


class weather_data(me.Document):
    time = me.StringField(default="")
    temp = me.StringField(default="")
    humi = me.StringField(default="")

#This route is acessed by the microcontroller only
@app.route("/upload_data", methods=["get"]) 
def upload_data():

    now = datetime.now()
    current_time = now.strftime("%Y-%m-%d %H:%M:%S")
    print(current_time)
    #This api recives data as get request and store them in the database
    time = current_time
    temp = request.args.get("temp")
    humi = request.args.get("humi")
    
    new_data = weather_data( #new_data is an object of weather_data
        time=time,
        temp=temp,
        humi=humi
    )
    
    new_data.save() #to save the new_data in the database
    return "done" # all api must have a return value

#This route is for accessing the data on database and displaying it on an html webpage
@app.route("/show_data", methods=["get"]) 
def show_data():
    data = weather_data.objects()
    print(data)
    return render_template("list_data.html", new_data = data)

@app.route("/clearAll", methods = ["post"])
def clear_all():
    data = weather_data.objects()
    data.delete()
    if data:
        print("Data cleared")
    else:
        print("not cleared")
    return render_template("list_data.html", new_data = data)


app.run(debug=True)