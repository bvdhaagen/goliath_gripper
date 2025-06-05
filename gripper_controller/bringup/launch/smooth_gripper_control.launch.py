#!/usr/bin/env python3
import rclpy
from rclpy.node import Node
from std_msgs.msg import Float64MultiArray
import time

class SmoothGripper(Node):
    def __init__(self):
        super().__init__('smooth_gripper')
        self.pub = self.create_publisher(Float64MultiArray, '/position_controller/commands', 10)
        
    def run(self):
        while rclpy.ok():
            # Smoothly open (50 → -50)
            for pos in range(150, 49, -5):  # Step down from 150 to 50
                self.pub.publish(Float64MultiArray(data=[float(pos), float(-pos)]))
                time.sleep(0.1)
            
            # Smoothly close (50 → 150)
            for pos in range(50, 151, 5):  # Step up from 50 to 150
                self.pub.publish(Float64MultiArray(data=[float(pos), float(-pos)]))
                time.sleep(0.1)

def main():
    rclpy.init()
    node = SmoothGripper()
    node.run()
    rclpy.shutdown()

if __name__ == '__main__':
    main()
