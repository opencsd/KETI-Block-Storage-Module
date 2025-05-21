import tkinter as tk
from http.server import BaseHTTPRequestHandler, HTTPServer
import threading
import json
from urllib.parse import urlparse, parse_qs
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import queue

# 전역 변수
scan_count = 0
filter_count = 0
row_mutex = threading.Lock()  # Mutex 객체

# 그래프 관련 전역 변수
fig = None
ax = None
canvas = None
gui_ready_event = threading.Event()  # Event to signal when GUI is ready
update_queue = queue.Queue()  # Queue to safely send updates to the GUI

# HTTP 서버 요청 처리 클래스
class RequestHandler(BaseHTTPRequestHandler):
    def _send_response(self, data):
        self.send_response(200)
        self.send_header('Content-type', 'application/json')
        self.end_headers()
        self.wfile.write(json.dumps(data).encode())

    def do_GET(self):
        global scan_count, filter_count
        # URL에서 쿼리 파라미터 추출
        parsed_url = urlparse(self.path)
        query_params = parse_qs(parsed_url.query)
        
        # /scan 엔드포인트 처리
        if self.path.startswith('/scan'):
            value = query_params.get('value', [None])[0]
            if value is not None and value.isdigit():
                with row_mutex:  # mutex로 보호
                    scan_count += int(value)
            update_queue.put("scan")  # Send signal to update graph

        # /filter 엔드포인트 처리
        elif self.path.startswith('/filter'):
            value = query_params.get('value', [None])[0]
            if value is not None and value.isdigit():
                with row_mutex:  # mutex로 보호
                    filter_count += int(value)
            update_queue.put("filter")  # Send signal to update graph

        else:
            self.send_response(404)
            self.end_headers()

# Tkinter GUI
def create_gui():
    global ax, canvas, fig

    root = tk.Tk()
    root.title("CSD Metric Visualizer")
    root.geometry("600x400")  # 초기 크기 설정

    # 그래프를 표시할 프레임
    graph_frame = tk.Frame(root)
    graph_frame.pack(fill=tk.BOTH, expand=True)

    # Matplotlib 그래프 초기화
    fig, ax = plt.subplots(figsize=(5, 3))
    canvas = FigureCanvasTkAgg(fig, master=graph_frame)
    canvas.get_tk_widget().pack(fill=tk.BOTH, expand=True)
    
    # 그래프 초기 그리기
    ax.barh(['Filter', 'Scan'], [filter_count, scan_count], color=['#E7B3D9', '#87D3F5'])
    
    ax.set_xlim(0, 600100000)
    # ax.set_xlabel('Count')
    # ax.set_title('Scan/Filter Row Count')

    # 가로 축 표시
    ax.xaxis.set_visible(False)

    canvas.draw()

    # GUI 준비가 완료되었음을 신호
    gui_ready_event.set()

    # 그래프를 1초마다 업데이트하는 함수
    def update_periodically():
        if not update_queue.empty():
            signal = update_queue.get()
            update_graph(signal)
        root.after(1000, update_periodically)  # 1000ms = 1초마다 update_periodically 실행

    root.after(1000, update_periodically)  # 1초마다 그래프 업데이트 시작
    
    # 창 크기 변경 시 그래프 비율 자동 조정
    def on_resize(event):
        fig.set_size_inches(event.width / 100, event.height / 100)
        canvas.draw()

    # Bind the resize event to adjust the graph size dynamically
    root.bind("<Configure>", on_resize)

    root.mainloop()

# 그래프 업데이트 함수
def update_graph(signal):
    global scan_count, filter_count, ax, canvas

    # 이전 그래프 지우기
    ax.clear()

    # 그래프 데이터 업데이트
    with row_mutex:  # mutex로 보호
        bars = ax.barh(['Filter', 'Scan'], [filter_count, scan_count], color=['#E7B3D9', '#87D3F5'])

    ax.set_xlim(0, 600100000)
    # ax.set_xlabel('Count')
    # ax.set_title('Scan/Filter Row Count')

    # 가로 축 표시
    ax.xaxis.set_visible(False)

    # 각 바에 숫자 표시 (그래프 중앙에)
    flag = False
    for bar in bars:
        width = bar.get_width()
        if flag:
            ax.text(width / 2, bar.get_y() + bar.get_height() / 2, f'{width}', va='center', ha='center')
        else:
            ax.text(width + 1, bar.get_y() + bar.get_height() / 2, f'{width}', va='center', ha='left')
        flag = True

    # 변경된 그래프 다시 그리기
    canvas.draw()

# HTTP 서버 실행 함수
def run_http_server():
    server_address = ('', 9002)  # 포트 9002 실행
    httpd = HTTPServer(server_address, RequestHandler)
    print("Server running on port 9002...")
    httpd.serve_forever()

if __name__ == '__main__':
    server_thread = threading.Thread(target=run_http_server)
    gui_thread = threading.Thread(target=create_gui)

    gui_thread.start()
    server_thread.start()
